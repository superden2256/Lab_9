#define _CRT_NON_CONFORMING_WCSTOK
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <tchar.h>
#include <string>
#include <iostream>

#include <chrono>
#include <vector>

#include <conio.h>
#include <string>

std::vector<std::wstring> messages;

int main(int argc, char* argv[]) {

    std::string path = "\\\\.\\pipe\\road" + std::string(argv[1]);

    HANDLE hPipe = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hPipe == INVALID_HANDLE_VALUE) {
        _tprintf(_T("Could not open pipe (error %d)\n"), GetLastError());
        return 1;
    }

    // Створення file mapping об'єкта (транслятора файлу)
    HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, L"SharedMemory");
    if (hMapFile == NULL) {
        wprintf(L"Could not open file mapping (error %d)\n", GetLastError());
        //CloseHandle(hPipe);
        return 1;
    }

    // Отримання вказівника на віртуальний адресний простір (file mapping)
    LPWSTR pBuf = (LPWSTR)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 1024);
    if (pBuf == NULL) {
        wprintf(L"Could not map view of file (error %d)\n", GetLastError());
        CloseHandle(hMapFile);
        //CloseHandle(hPipe);
        return 1;
    }

    auto startTime = std::chrono::steady_clock::now();
    auto endTime = startTime + std::chrono::seconds(8);
    while ((std::chrono::steady_clock::now() < endTime)) {
        if (std::chrono::steady_clock::now() < endTime) {
            wprintf(L"Enter a message: ");

            wchar_t newInput[1024];
            //std::wcin >> newInput;

            std::wcin.getline(newInput, 1024);


            if (wcscmp(newInput, L"End") == 0) {
                std::wcout << "Exiting the program\n";
                break;
            }
            else {
                HANDLE mutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, TEXT("OurMutex"));
                if (mutex == NULL) {
                    wprintf(L"Failed to open mutex (error %d)\n", GetLastError());
                }
                wcscat(pBuf, newInput);
                wcscat(pBuf, L"\n");
                ReleaseMutex(mutex);
            }
        }
    }

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

    // Вводяться голоси
    wchar_t votes[1024];
    std::cout << "Choose 3 best ideas: \n";
    std::wcin.getline(votes, 1024);
    WriteFile(hPipe, votes, sizeof(votes), NULL, NULL);

    // Закриття ресурсів
    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);

    SuspendThread(GetCurrentThread());

    return 0;
    
}