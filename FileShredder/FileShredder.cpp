// FileShredder.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <vector>
#include <windows.h>
#include <random>
#include <algorithm>

std::mt19937 rng(std::random_device{}());

std::string CleanPath(std::string path)
{
	if (path.empty())
		return "";
	if (path.front() == '"')
		path.erase(0, 1);
	if (path.back() == '"')
		path.pop_back();
	return path;
}

std::string GenerateRandomName(int length)
{
	const std::string characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
	std::uniform_int_distribution<> dist(0, characters.size() - 1);
	std::string randomName;
	for (int i = 0; i < length; ++i)
	{
		randomName += characters[dist(rng)];
	}
	return randomName;
}

bool OverwritePass(HANDLE hFile, long long fileSize, int patternType, int passNumber)
{
	SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

	const DWORD BUFFER_SIZE = 4096;
	std::vector<char> buffer(BUFFER_SIZE);

	if (patternType == 0)
	{
		std::fill(buffer.begin(), buffer.end(), 0x00);
	}
	else if (patternType == 1)
	{
		std::fill(buffer.begin(), buffer.end(), (char)0xFF);
	}

	std::uniform_int_distribution<> dist(0, 255);
	long long bytesRemaining = fileSize;
	DWORD bytesWritten = 0;

	std::cout << "   -> Pass " << passNumber << "/3 running...";

	while (bytesRemaining > 0)
	{
		DWORD toWrite = (bytesRemaining < BUFFER_SIZE) ? (DWORD)bytesRemaining : BUFFER_SIZE;

		if (patternType == 2)
		{
			for (DWORD i = 0; i < toWrite; ++i)
			{
				buffer[i] = static_cast<char>(dist(rng));
			}
		}

		if (!WriteFile(hFile, buffer.data(), toWrite, &bytesWritten, NULL))
		{
			std::cerr << "[ERROR] Write failed ! \n";
			return false;
		}

		bytesRemaining -= bytesWritten;
	}
	FlushFileBuffers(hFile);
	std::cout << "OK.\n";
	return true;
}

int main()
{
	SetConsoleTitleA("File Shredder");
	system("color 0C");

	std::cout << "=====================================\n";
	std::cout << "            File Shredder            \n";
	std::cout << "             Version 0.2             \n";
	std::cout << "=====================================\n";

	while (true)
	{
		std::cout << "Please Drag & Drop your file here (or write 'exit'):\n";

		std::string inputPath;
		std::getline(std::cin, inputPath);

		if (inputPath == "exit")
			break;

		std::string filePath = CleanPath(inputPath);

		std::cout << "\n[INFO] Trying to open the file: " << filePath << "...\n";

		HANDLE hFile = CreateFileA(
			filePath.c_str(),
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			DWORD error = GetLastError();
			std::cerr << "[ERROR] Access denied! Error Code: " << error << "\n";

			if (error == 32)
				std::cerr << "-> File is being used by another program.\n";
		}
		else
		{

			LARGE_INTEGER FileSize;
			if (!GetFileSizeEx(hFile, &FileSize))
			{
				std::cerr << "[Error] Could not get Filesize.\n";
				CloseHandle(hFile);
				continue;
			}

			std::cout << "[INFO] Size: " << FileSize.QuadPart << "Bytes. \n";
			std::cout << "[WARNING] Are you sure you want to perform 3-Pass DoD overwrite? (y/n): ";

			std::string confirm;
			std::getline(std::cin, confirm);

			if (confirm != "y")
			{
				std::cout << "[ABORT] Operation cancelled. \n";
				CloseHandle(hFile);
				continue;
			}

			std::cout << "[Processing] Starting DoD overwrite...\n";

			if (!OverwritePass(hFile, FileSize.QuadPart, 0, 1))
				break; // Pass 1: Zeros
			if (!OverwritePass(hFile, FileSize.QuadPart, 1, 2))
				break; // Pass 2: Ones
			if (!OverwritePass(hFile, FileSize.QuadPart, 2, 3))
				break; // Pass 3: Random

			std::cout << "[SUCCESS] File Content destroyed. \n";

			CloseHandle(hFile);

			std::cout << "\n[INFO] Obfuscating file name... \n";

			size_t lastSlash = filePath.find_last_of("\\/");
			std::string directory = (lastSlash == std::string::npos) ? "" : filePath.substr(0, lastSlash + 1);
			std::string newName = directory + GenerateRandomName(12) + ".tmp";

			if (MoveFileA(filePath.c_str(), newName.c_str()))
			{
				std::cout << "[SUCCESS] Renamed to: " << newName << "\n";
			}
			else
			{
				std::cerr << "[ERROR] Could not rename file. Error: " << GetLastError() << "\n";
			}
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
