# TX36

旗標科技文字檔轉 TX 排版檔工具程式

|檔案|說明|編譯器|輸入|輸出|
|----|-----|----|----|----|
|TX36.c|原始 big5 編碼的版本|[Borland C Compiler v5.5](https://developerinsider.co/download-and-install-borland-c-compiler-on-windows-10/)|big5 編碼檔|big5 編碼檔|
|TXgcc36.c|big5 編碼的版本, 替換掉 DOS 時代的 _dos_findfirst 和 _dos_findnext 函式, 改用 _findfirst 和 _findnext|[TDM-GCC v10.3.0](https://jmeubank.github.io/tdm-gcc/about/)|big5 編碼檔|big5 編碼檔|
|TXu36.c|將原始碼改存成 utf-8 編碼的版本, 用最偷懶的方式讓此工具可處理 utf-8 編碼的檔案,以便可以在稿件中使用 unicode 字元 <br>(不要搭配 txm 自動加空白的功能, 因為自動加空白的功能是 big5 碼工具, 不認得 UTF-8, 再加粗英數字的地方會出錯)|[TDM-GCC v10.3.0](https://jmeubank.github.io/tdm-gcc/about/)<br>[Embarcadero Dev C++ 6.3(TDM-GCC 9.2.0)](https://www.embarcadero.com/free-tools/dev-cpp)|utf-8 編碼檔|utf-8 編碼檔|

Borland C Compiler 使用時必須將所在路徑加入 PATH 環境變數, 不然編譯時會發生找不到 ilink 連結程式。此外, 也要參考 TxBcc55.bat 檔內容設定表頭檔以及程式庫路徑, 或者也可以在該路徑下建立設定檔：

- bcc32.cfg

    ```
    -I"d:\dev\Bcc55\include"
    -L"d:\dev\Bcc55\lib"
    ```
- ilink32.cfg

    ```
    -L"d:\dev\Bcc55\lib"
    ```
    
