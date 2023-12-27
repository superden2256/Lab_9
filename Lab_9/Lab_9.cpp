#define _CRT_NON_CONFORMING_WCSTOK
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <tchar.h>
#include <iostream>
#include <vector>
#include <string>
#include <codecvt>
#include <sstream>
#include <algorithm>

// Вектор використовується для збереження ідей
std::vector<std::wstring> messages;
std::vector<std::string> votesArray;
std::vector<HANDLE> pipes;

int wmain(int argc, wchar_t* argv[]) {

    HANDLE mutex = CreateMutexA(NULL, FALSE, "OurMutex");

    // Створення файлу для file mapping
    // Через нього і відбувається обмін повідомленнями
    HANDLE hFile = CreateFile(L"SharedFile.txt", GENERIC_READ | GENERIC_WRITE,
        0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        wprintf(L"Could not create file (error %d)\n", GetLastError());
        return 1;
    }
      
    // Створення file mapping об'єкта (транслятора файлу)
    HANDLE hMapFile = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, 1024, L"SharedMemory");
    if (hMapFile == NULL) {
        wprintf(L"Could not create file mapping (error %d)\n", GetLastError());
        CloseHandle(hFile);
        return 1;
    }

    // Отримання вказівника на віртуальний адресний простір
    LPWSTR pBuf = (LPWSTR)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 1024);
    if (pBuf == NULL) {
        wprintf(L"Could not map view of file (error %d)\n", GetLastError());
        CloseHandle(hMapFile);
        CloseHandle(hFile);
        return 1;
    }

    // Отримання кількості клієнтів від користувача
    int numClients;
    wprintf(L"Enter the number of client processes to create: ");
    wscanf(L"%d", &numClients);

    // Вектор для збереження дескрипторів клієнтських процесів
    std::vector<HANDLE> clientProcesses;
    for (int i = 0; i < numClients; i++) {

        std::string path = "\\\\.\\pipe\\road" + std::to_string(i);

        // Крок 1: Підключення до іменованої труби
        HANDLE hPipe = CreateNamedPipeA(path.c_str(), PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 1024, 1024, NMPWAIT_USE_DEFAULT_WAIT, NULL);
        if (hPipe != INVALID_HANDLE_VALUE) {
            pipes.push_back(hPipe);
        }

        // Запуск клієнта як дочірнього процесу
        STARTUPINFO si = { sizeof(LPSTARTUPINFO) };
        PROCESS_INFORMATION pi;

        std::wstring commandLine = L"..\\x64\\Debug\\Lab_9_client.exe " + std::to_wstring(i);

        if (CreateProcess(NULL, (LPWSTR)commandLine.data(), NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
            clientProcesses.push_back(pi.hProcess);

            // Очікування підключення клієнта
            wprintf(L"Waiting for client to connect...\n");
            wprintf(L"Client #%d connected\n", i+1);
        }
        else {
            wprintf(L"Failed to start client process (error %d)\n", GetLastError());
        }

    }

    WaitForMultipleObjects(clientProcesses.size(), clientProcesses.data(), TRUE, 20000);

    // Розділення стрічки на речення
    wchar_t* temp = new wchar_t[1024];
    wcscpy(temp, pBuf);

    wchar_t* token = wcstok(temp, L"\n");

    // Запис ідеї у vector
    while (token != nullptr) {
        messages.push_back(token);
        token = wcstok(nullptr, L"\n");
    }

    for (int i = 0; i < messages.size(); i++) {
        std::wcout << i + 1 << ") " << messages[i] << "\n";
    }

    // Зчитування голосів
    wchar_t votes[1024];

    // Набір голосів
    for (int i = 0; i < clientProcesses.size(); i++) {
        ReadFile(pipes[i], votes, sizeof(votes), NULL, NULL);
        std::wcout << votes << std::endl;

        // Використовуємо std::wstring_convert для конвертації
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        std::string votes_str = converter.to_bytes(votes);

        votesArray.push_back(votes_str);
    }

    std::vector<int> countOfVotes(messages.size());

    // Вибираємо три найкращі ідеї
    for (int i = 0; i < votesArray.size(); i++) {
        std::istringstream inputStream(votesArray[i]);
        int number1, number2, number3;
        inputStream >> number1 >> number2 >> number3;

        for (int j = 0; j < messages.size(); j++) {
            if (number1 == j)
                countOfVotes[number1]++;
            if (number2 == j)
                countOfVotes[number2]++;
            if (number3 == j)
                countOfVotes[number3]++;
        }
    }

    // Вивід найкращих ідей
    // Сортування вектора за спаданням
    std::cout << "\n\nThe best ideas:\n";
    std::sort(countOfVotes.begin(), countOfVotes.end(), std::greater<int>());

    for (int i = 0; i < 3 && i < countOfVotes.size(); ++i) {
        std::wcout << messages[i] << "- ";
        std::cout << countOfVotes[i] << " votes\n";
    }

    for (int i = 0; i < clientProcesses.size(); i++) {
        CloseHandle(clientProcesses[i]);
    }

    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);
    CloseHandle(hFile);

    return 0;
}
