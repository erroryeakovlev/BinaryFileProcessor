# Binary File Processor

   C++/Qt         XOR   8- .

          .

## 

*     ;
*     ;
*    ;
*     `;`;
*   `.txt`, `*.txt`, `testFile.bin`   ;
* XOR  8- ,    HEX;
*    ;
*       ;
*      ;
*   ;
*     ;
*   ;
*   ;
*   ;
*    ;
*  ;
*      ;
*         .

## XOR

  16  , :

```text
1234567890ABCDEF
```

   8- :

```text
12 34 56 78 90 AB CD EF
```

     :

```text
input[0] XOR 0x12
input[1] XOR 0x34
input[2] XOR 0x56
...
input[7] XOR 0xEF
input[8] XOR 0x12
...
```

##    

     .

    1 MiB:

```text
 
    
1 MiB
    
XOR
    

    
 1 MiB
    
...
```

    `QThread`,   GUI-       ,    10 GB.

## Pause / Resume

    `std::condition_variable`.

  `Pause` worker      .

 `Resume`     .

##    

       :

```cpp
std::atomic_bool stopRequested;
std::atomic_bool pauseRequested;
```

  worker      .

  :

1.    ;
2.   `condition_variable`,  worker   ;
3.     worker  `QThread::wait()`.

##  

        `.part`.

        .

        .

##  

       .

   :

*  ;
*   .

    ,    .

  ,      .

##  

```text
.
 CMakeLists.txt
 main.cpp
 mainwindow.cpp
 mainwindow.h
 mainwindow.ui
 ProcessingWorker.cpp
 ProcessingWorker.h
 Fileprocessor.cpp
 Fileprocessor.h
 tests/
     FileProcessorSmokeTest.cpp
```

### `MainWindow`

 :

*  ;
*  ;
*  ;
*   ;
*  ;
*  worker;
*    .

### `ProcessingWorker`

 :

*   ;
*  `FileProcessor`;
* Pause / Resume;
* Stop;
*   ;
*    ;
* overwrite /  .

   `QThread`.

### `FileProcessor`

     .

    1 MiB,    XOR   8- .

## 

### 

* Qt 6.5  ;
* CMake 3.19  ;
* C++ compiler.

###   CMake

 Qt   :

```bash
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH=/path/to/Qt

cmake --build build
```

  macOS:

```bash
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH=/Users/USERNAME/Qt/6.11.2/macos

cmake --build build
```

## 

   smoke test  `FileProcessor`.

 :

*   ;
*  ;
*  8- XOR-;
*  ;
*   ;
*   ;
*    .

:

```bash
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH=/path/to/Qt

cmake --build build --target FileProcessorSmokeTest

ctest --test-dir build --output-on-failure
```

 :

```text
100% tests passed, 1 tests passed out of 1
```

## Windows / MinGW

   GitHub Actions workflow:

```text
.github/workflows/windows-mingw.yml
```

Workflow   Windows x64  :

1.  Qt;
2.  MinGW toolchain;
3.    CMake;
4.  ;
5.  `FileProcessorSmokeTest`;
6.  Qt runtime-  `windeployqt`;
7.   Windows artifact.

Windows-   :

```text
Build
  
FileProcessorSmokeTest
  
windeployqt
  
Windows artifact
```

##     Windows

   Windows-    Qt, Qt Creator, CMake  MinGW.

###  Windows-

1.     GitHub.
2.    **Actions**.
3.    workflow **Windows MinGW Build**.
4.       **Artifacts**.
5.   `BinaryFileProcessor-Windows`.
6.     .

       Qt runtime-.

 :

```text
BinaryFileProcessor-Windows/
 BinaryFileProcessor.exe
 Qt6Core.dll
 Qt6Gui.dll
 Qt6Widgets.dll
 ...
 platforms/
     qwindows.dll
```

### 

  :

```text
BinaryFileProcessor.exe
```

   `.exe`    ,      Qt-    `qwindows.dll`.

### 

   :

```text
Input directory        
Output directory       
File mask             
XOR value           16 HEX-
```

:

```text
Input directory:  C:\BinaryTest\Input
Output directory: C:\BinaryTest\Output
File mask:        *.bin
XOR value:        1234567890ABCDEF
```

      `;`:

```text
*.txt;*.bin;testFile.dat
```

     `*`:

```text
.txt
```

  :

```text
*.txt
```

###   

      **Start**.

          .

###  

    :

1.   **Timer**;
2.   ;
3.  **Start**.

       .

     ,         .

###  

   :

```text
Pause     
Resume    
Stop      
```

         worker-.

###    Windows

     :

* Qt 6.5  ;
* CMake 3.19  ;
* MinGW-compatible C++ compiler.

  :

```powershell
cmake -S . -B build
cmake --build build --config Release
```

     Qt- :

```powershell
windeployqt --release --compiler-runtime BinaryFileProcessor.exe
```

      Windows artifact  GitHub Actions.

## CI

GitHub Actions     Windows-.

 workflow :

*  CMake;
*  C++ ;
*   Windows/MinGW;
*  `FileProcessor`  CTest;
*  runtime- Qt.

     Windows artifact `BinaryFileProcessor-Windows`.

