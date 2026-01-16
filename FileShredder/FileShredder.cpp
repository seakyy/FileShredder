// FileShredder.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <vector>
#include <windows.h>


std::string CleanPath(std::string path)
{
	if (path.empty()) return "";
	if (path.front() == '"') path.erase(0, 1);
	if (path.back() == '"') path.pop_back();
	return path;
}


int main()
{
	SetConsoleTitleA("File Shredder");
	system("color 0C");

    std::cout << "=====================================\n";
	std::cout << "            File Shredder            \n";
	std::cout << "=====================================\n";

	while (true)
	{
		std::cout << "Please Drag & Drop your file here (or write 'exit'):\n";

		std::string inputPath;
		std::getline(std::cin, inputPath);

		if (inputPath == "exit") break;

		std::string filePath = CleanPath(inputPath);

		std::cout << "\n[INFO] Trying to open the file: " << filePath << "...\n";

		HANDLE hFile = CreateFileA(
			filePath.c_str(),
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			DWORD error = GetLastError();
			std::cerr << "[ERROR] Access denied! Error Code: " << error << "\n";

			if (error == 32) std::cerr << "-> File is being used by another program.\n";
		} else
		{

			LARGE_INTEGER FileSize;
			if (!GetFileSizeEx(hFile, &FileSize))
			{
				std::cerr << "[Error] Could not get Filesize.\n";
				CloseHandle(hFile);
				continue;
			}

			std::cout << "[INFO] Size: " << FileSize.QuadPart << "Bytes. \n";
			std::cout << "[WARNING] Are you sure you want to OVERWRITE this file with 0x00? (y/n): ";

			std::string confirm;
			std::getline(std::cin, confirm);

			if (confirm != "y")
			{
				std::cout << "[ABORT] Operation cancelled. \n";
				CloseHandle(hFile);
				continue;
			}

			std::cout << "[Processing] Overwriting data...";


			SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

			const DWORD BUFFER_SIZE = 4096; // Write in 4KB blocks, this is faster then writign byte for byte
			std::vector<char> zeros(BUFFER_SIZE, 0x00);

			long long bytesRemaining = FileSize.QuadPart;
			DWORD bytesWritten = 0;

			while (bytesRemaining > 0)
			{
				DWORD toWrite = (bytesRemaining < BUFFER_SIZE) ? (DWORD)bytesRemaining : BUFFER_SIZE;

				if (!WriteFile(hFile, zeros.data(), toWrite, &bytesWritten, NULL))
				{
					std::cerr << "\n[ERROR] Write failed during file shredding! \n";
					break;
				}

				bytesRemaining -= bytesWritten;
			}

			FlushFileBuffers(hFile);

			std::cout << " DONE!\n";
			std::cout << "[SUCCESS] File content has been destroyed. \n";

			CloseHandle(hFile);
		}

		std::cout << "\n----------------------------------------------\n\n";
	}

	return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
