---
title: Tutorial
---

# Building a DoD File Shredder in C++

In this tutorial, you will learn how to build a **Secure File Shredder** using C++ and the Windows API. Unlike the usual "delete" operation, which just removes the reference to a file, this tool overwrites the data on the file or to be precise the disk multiple times to prevent forensic recovery.

We will implement the [**DoD 5220.22-M** standard](https://blancco.com/resources/blog-dod-5220-22-m-wiping-standard-method/) (3-pass overwrite) and add anti forensic features like random file renaming and colored console output for better visibility.

## Prerequisites

Before you start, you must:
* Known the basics of C++ : Vectors, I/O, Strings
* Have configurated with the C++ Workload Visual Studio Code / Visual Studio
* Have a Windows OS, because we use `windows.h`.

---

## Supported File Types

This tool operates on the binary level which is raw bytes. Meaning it technically works with any file. However, for the purpose of this release, the following formats have been **explicitly tested and verified to work with**:

### Tested & Supported
* **Documents:** `.txt`, `.md`, `.pptx`
* **Media:** `.jpg`, `.mp4`
* **Executables:** `.exe`
* **System:** `.tmp`

### Untested
The following formats have **not** been extensively tested. While the shredding algorithm should in theory work, file locks or OS specific handlers might interfere in the process:
* `.pdf`
* `.svg`
* `.zip` (and other archives)

---

## Core Concepts

We will focus on three main concepts to make this tool secure:

1.  **Exclusive Access:** Using `CreateFile` with `0` sharing mode to ensure no other process interrupts us.
2.  **The Overwrite Engine:** Using `SetFilePointer` and `WriteFile` to overwrite every byte of the file.
3.  **Cache Bypassing:** Using `FlushFileBuffers` to force the OS to write data from RAM to the physical disk immediately.

---

## Step 1: The Overwrite Logic (DoD Standard)

The core of our application is the `OverwritePass` function. It takes a file handle and fills it with specific patterns.

**The 3 Pass Algorithm:**
1.  **Pass 1:** All Zeros (`0x00`)
2.  **Pass 2:** All Ones (`0xFF`)
3.  **Pass 3:** Random Data (using `std::mt19937`)

```cpp
bool OverwritePass(HANDLE hFile, long long fileSize, int patternType, int passNumber)
{
    // 1. Reset Cursor to the beginning
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

    const DWORD BUFFER_SIZE = 4096; // 4KB Chunks for better performance
    std::vector<char> buffer(BUFFER_SIZE);

    // 2. Prepare the Pattern
    if (patternType == 0) std::fill(buffer.begin(), buffer.end(), 0x00);      // Zeros
    else if (patternType == 1) std::fill(buffer.begin(), buffer.end(), (char)0xFF); // Ones
    
    // ... (Loop to write data) ...

    // 3. Force write to disk
    FlushFileBuffers(hFile); 
    return true;
}
```

## Step 2: Anti Forensics (Obfuscation)

If we just delete _SecretPlans.txt_, the file name might still exist in the Master File Table. To counter this, we rename the file to a random string before deleting it.

```cpp
// Generates a random name like "X7k9L2.tmp"
std::string newName = directory + GenerateRandomName(12) + ".tmp";

// Rename the file on disk
MoveFileA(filePath.c_str(), newName.c_str());

// Finally, delete the random .tmp file
DeleteFileA(newName.c_str());
```

## Step 3: User Experience & Colors

We use the Windows Console API (SetConsoleTextAttribute) to give visual feedback. This prevents mistakes and upgrade usability.
* [INFO] (Cyan): General status updates.
* [WARNING] (Yellow): Safety confirmation prompts.
* [SUCCESS] (Green): Operations completed successfully.
* [ERROR] (Red): Critical failures.



## Expected Result

When running the application, dragging a file into the console, and confirming with y, you should see the following sequence:

<img width="669" height="524" alt="grafik" src="https://github.com/user-attachments/assets/13f4b1b9-3fba-4f03-99e1-1feb7ea12dce" />

_(Note: The file is now physically unrecoverable.)_


## Troubleshooting: What can go wrong?
Even with a perfect algorithm, Windows might block the operation. These are the most common errors:
**1. Error Code 32: "The process cannot access the file..."**
* **Cause:** The file is open in another program.
* **Solution:** Close the application using the file and try again.

**2. Error Code 5: "Access Denied"**
* **Cause:** You are trying to shred a system file or a file in a protected folder without Administrator privileges.
* ~~**Solution:** Right-click FileShredder.exe and select "Run as Administrator".~~ *Only run this with Administrator privileges if you know what you are doing!*

**3. Renaming Fails**
* **Cause:** Sometimes antivirus software blocks the renaming of files that are currently being modified rapidly.
* **Fallback:** The application includes a fallback mechanism: if renaming fails, it attempts to delete the original file directly to ensure security is maintained.


# License & Disclaimer

This tool is for educational purposes. The author is not responsible for any data loss. Once shredded, data cannot be recovered.

[View Source Code on GitHub](https://github.com/seakyy/FileShredder)
