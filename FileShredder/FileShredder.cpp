// FileShredder.cpp : This file contains the 'main' function. Program execution begins and ends there.
// works with formats like .txt, .md, .exe, .jpg, .pptx, .mp4, .tmp
// limits: .pdf, .svg, .zip,

#include <iostream>
#include <string>
#include <vector>
#include <windows.h>
#include <random>
#include <algorithm>

std::mt19937 rng(std::random_device{}());

const int COLOR_DEFAULT = 7;
const int COLOR_GREEN = 10;
const int COLOR_RED = 12;
const int COLOR_YELLOW = 14;
const int COLOR_CYAN = 11;
const int COLOR_MAGENTA = 13;

void Log(std::string tag, std::string message)
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	int color = COLOR_DEFAULT;

	if (tag == "[INFO]")
		color = COLOR_CYAN;
	else if (tag == "[SUCCESS]")
		color = COLOR_GREEN;
	else if (tag == "[WARNING]")
		color = COLOR_YELLOW;
	else if (tag == "[ERROR]")
		color = COLOR_RED;
	else if (tag == "[Processing]")
		color = COLOR_MAGENTA;
	else if (tag == "[ABORT]")
		color = COLOR_RED;

	SetConsoleTextAttribute(hConsole, color);
	std::cout << tag;

	SetConsoleTextAttribute(hConsole, COLOR_DEFAULT);
	std::cout << " " << message << "\n";
}

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

	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, COLOR_DEFAULT);
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
			Log("[ERROR]", "Write failed!");
			return false;
		}

		bytesRemaining -= bytesWritten;
	}
	FlushFileBuffers(hFile);
	SetConsoleTextAttribute(hConsole, COLOR_GREEN);
	std::cout << "OK.\n";
	SetConsoleTextAttribute(hConsole, COLOR_DEFAULT);
	return true;
}

int main()
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, COLOR_RED);
	std::cout << "=====================================\n";
	SetConsoleTextAttribute(hConsole, COLOR_DEFAULT);
	std::cout << "            File Shredder            \n";
	std::cout << "             Version 1.1             \n";
	SetConsoleTextAttribute(hConsole, COLOR_RED);
	std::cout << "=====================================\n";
	SetConsoleTextAttribute(hConsole, COLOR_DEFAULT);

	while (true)
	{
		std::cout << "Please Drag & Drop your file here (or write 'exit'):\n";

		std::string inputPath;
		std::getline(std::cin, inputPath);

		if (inputPath == "exit")
			break;

		std::string filePath = CleanPath(inputPath);

		std::cout << "\n";
		Log("[INFO]", "Trying to open the file: " + filePath + "...");

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
			Log("[ERROR]", "Access denied! Error Code: " + std::to_string(error));
			if (error == 32)
				Log("[ERROR]", "-> File is being used by another program.");
		}
		else
		{
			LARGE_INTEGER FileSize;
			if (!GetFileSizeEx(hFile, &FileSize))
			{
				Log("[ERROR]", "Could not get Filesize.");
				CloseHandle(hFile);
				continue;
			}

			Log("[INFO]", "Size: " + std::to_string(FileSize.QuadPart) + " Bytes.");
			SetConsoleTextAttribute(hConsole, COLOR_YELLOW);
			std::cout << "[WARNING]";
			SetConsoleTextAttribute(hConsole, COLOR_DEFAULT);
			std::cout << " Are you sure you want to SHRED this file? (y/n): ";

			std::string confirm;
			std::getline(std::cin, confirm);

			if (confirm != "y")
			{
				Log("[ABORT]", "Operation cancelled.");
				CloseHandle(hFile);
				continue;
			}

			Log("[Processing]", "Starting DoD overwrite...");

			if (!OverwritePass(hFile, FileSize.QuadPart, 0, 1))
				break; // Pass 1: Zeros
			if (!OverwritePass(hFile, FileSize.QuadPart, 1, 2))
				break; // Pass 2: Ones
			if (!OverwritePass(hFile, FileSize.QuadPart, 2, 3))
				break; // Pass 3: Random

			Log("[SUCCESS]", "File Content destroyed.");

			CloseHandle(hFile);

			Log("[INFO]", "Obfuscating file name...");

			size_t lastSlash = filePath.find_last_of("\\/");
			std::string directory = (lastSlash == std::string::npos) ? "" : filePath.substr(0, lastSlash + 1);
			std::string newName = directory + GenerateRandomName(12) + ".tmp";

			if (MoveFileA(filePath.c_str(), newName.c_str()))
			{
				Log("[SUCCESS]", "Renamed to: " + newName);

				if (DeleteFileA(newName.c_str()))
				{
					Log("[SUCCESS]", "FINAL: File deleted permanently.");
				}
				else
				{
					Log("[ERROR]", "Could not delete temp file. Error: " + std::to_string(GetLastError()));
				}
			}
			else
			{
				Log("[ERROR]", "Could not rename file. Error: " + std::to_string(GetLastError()));
				Log("[INFO]", "Trying to delete original file instead...");

				if (DeleteFileA(filePath.c_str()))
				{
					Log("[SUCCESS]", "FINAL: Original file deleted permanently.");
				}
				else
				{
					Log("[ERROR]", "Could not delete original file either. Error: " + std::to_string(GetLastError()));
				}
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
