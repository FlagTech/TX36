#include <stdio.h>
#include <stdlib.h>
#include <dir.h>
#include <dos.h>
// #include <bios.h>
#include <io.h>
#include <string.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <locale.h>
#include <conio.h>
#include <ctype.h>

char version[10] = "3.6";

typedef unsigned char byte;
void Process(void);
void CheckDir(byte *dir);
void error(char *msg);
int getyna(void);
void CutDir(void);
void MakeName(byte *from, byte *tm, byte *name);
void Convert(char *inFile, char *outFile);

struct find_t fs, fs2;
FILE *ifp, *ofp;
byte Template[20];
byte outDir[96], inDir[96], tmpbuf[9];
byte outPath[96], inPath[96], drive[3], dir[88], name[24], ext[4];
char sys,           // 是否要處理子資料夾
     over,          // 是否要覆蓋已經存在的輸出檔
     story = 1;     // 是否要家輸出的每一行前面加上樣式名稱
int cnt;

void help(void)
{
   printf("\nTx /snnn [SourceFile [SourceFile...]]"
          "\n       snnn is book number, eg. f243, f8201a"
          "\n!! your source file must be CH??.TXT, eg. CH05.TXT"
          "\n!! Use < Tx //snnn ... > will also search subdirectories"
          "\n"
          "\n------------------- Advanced usage --------------------"
          "\nTx Source [Source...] OutputDir [/S] [/Nst] [/O] [/P]"
          "\n  /S: include all Sub-directory"
          "\n  /Nst: Set output filename"
          "\n      st: String Template    "
          "\n        \\? - char at n pos in SourceFile's name"
          "\n      eg: ch02.txt==>/NF100-\\2\\3.txt==>F100-02.txt"
          "\n  /O: Overwrite exist files without asking"
          "\n  /P: Don't add Style in output files"
          "\n  * be careful when OutputDir under current Dir."
          "\n-------------------------------------------------------");
   getch();
   exit(0);
}

void main(int argc, char *argv[])
{
   int idx, i, j;
   //   setlocale(LC_ALL,"950");
   printf("\n++++++++++++++++++++++++++++++++++++++●○●+++++++++++++++"
          "\nKen's text tool for PageMaker 6.xC                 ver %s"
          "\nTx (C) CopyRight 1997-2003                         by Ken L"
          "\n+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n",
          version);
   if (argc > 1 && (argv[1][0] == '/' || argv[1][0] == '!')) // 有指定書號
   {
      if (argv[1][1] != '/')             // tx /FXXXX
         strcpy(Template, argv[1] + 1);
      else                               // tx //FXXXX
      {
         sys = 1;                        // 包含子資料夾
         strcpy(Template, argv[1] + 2);
      }
      if (argv[1][0] == '!')
         story = 0;
      if (strlen(Template) > 6)          // 書號太長 
         help();
      if (strlen(Template) < 6)
         strcat(Template, "-");
      strcat(Template, "\\2\\3.txt");    // 輸出檔名為 "書號-章序.txt" 的格式
      strcpy(outDir, ".");               // 預設輸出資料夾為目前資料夾
      if (argc == 2)                     // tx /FXXXX 未指定檔名
      {
         strcpy(inDir, "ch??.txt");      // 搜尋所有 "chxx" 的檔名
         Process();
      }
      else                               // tx /FXXXX chxx.txt ....
      {
         for (i = 2; i < argc; i++)      // 一一處理命令列中指定的檔名
         {
            strcpy(inDir, argv[i]);
            Process();
         }
      }
   }
   else                                      // <F4>沒有指定書號
   {
      for (idx = argc - 1; idx > 0; idx--)   // 從命令列尾端往為找到指定輸出路徑的參數
         if (argv[idx][0] != '/')
            break;
      for (i = idx - 1; i > 0; i--)          // 從輸出路徑再往回找最後一個輸路徑的參數
         if (argv[i][0] != '/')
            break;
      if (idx == 0 || i == 0)                // 沒有指定輸出路徑或是輸入路徑
         help();                             // 參數錯誤, 顯示說明給使用者參考

      for (i = argc - 1; i > 0; i--)         // 從命令行尾端往回找出輸出選項
         if (argv[i][0] == '/')
            switch (argv[i][1] | 0x20)       // 把大小字母變小寫 0x65 | 0x20 => 0x45
            {
            case 's':                        // 包含子資料夾
               sys = 1;
               break;
            case 'n':                        // 指定檔名樣版
               strcpy(Template, argv[i] + 2);
               break;
            case 'o':
               over = 1;                     // 覆蓋已存在的輸出檔
               break;
            case 'p':
               story = 0;                    // 不附加樣式
               break;
            default:
               help();
            }

      strcpy(outDir, argv[idx]);             // 設定輸出路徑
      i = strlen(outDir) - 1;                // 移除路徑尾端的 '\'
      if (outDir[i] == '\\')
         outDir[i] = 0;
      CheckDir(outDir);                      // 檢查輸出路徑, 若不存在就建立該資料夾
      for (j = 1; j < idx; j++)              // 一一找出來原路徑
      {
         if (argv[j][0] != '/')
         {
            strcpy(inDir, argv[j]);
            Process();                       // 處理目前這一個來源路徑
         }
      }
   }
   printf("\n\n%d file(s) process O.K.\n", cnt);
   exit(0);
}

void Process()
{
   int i, done;

   done = _dos_findfirst(inDir, _A_RDONLY | _A_HIDDEN | _A_SYSTEM, &fs);

   while (!done)
   {
      // 用找到的檔名取代可能含有萬用字元的來源檔名
      _splitpath(inDir, drive, dir, name, ext);
      _makepath(inPath, drive, dir, fs.name, "");

      // 建立輸出檔路徑
      if (Template[0] == 0)       // 沒有指定書號時不會有樣版
      {
         strcpy(name, fs.name);   // 用來源檔名字首改 'x' 後當輸出檔名
         name[0] = 'x';
      }
      else // 用找到的檔名中的字元替換掉樣版中的 \x 佔位字元
         MakeName(fs.name, Template, name);
      ext[0] = 0;
      _makepath(outPath, "", outDir, name, ext); // 建立輸出檔名

      // 未指定覆蓋輸出檔而且輸出檔已經存在
      if ((over == 0) && (ofp = fopen(outPath, "rb")) != NULL)
      {  // 詢問是否要覆蓋輸出檔
         printf("\n%s already exist! overwrite? (Yes/No/All)", outPath);
         i = getyna(); // 讓使用者按鍵選取作法
         if (i == 'n')
            goto skip;
         if (i == 'a')
            over = 1;
      }

      cnt++;

      // 顯示目前要處理的來源檔與輸出檔名
      printf("\n%s ==> %s", inPath, outPath);
      Convert(inPath, outPath);   // 處理目前檔案
   skip:
      done = _dos_findnext(&fs);  // 找下一個檔
   }

   if (sys)                       // 如果要包含子資料夾
   {
      byte dircnt, i;
      dircnt = 0;
      // printf("\nsys: in %s,  out %s", inDir, outDir);
      while (1)
      {
         // 組合出 "目前的路徑\*.*:find directory
         _splitpath(inDir, drive, dir, name, ext);
         _makepath(inPath, drive, dir, "*", "*");
         // printf("\ninpth dir %s", inPath);
         // 只搜尋資料夾
         done = _dos_findfirst(inPath, FA_DIREC, &fs2);
         for (i = 0; i < dircnt; i++)
            done = _dos_findnext(&fs2);
         if (done)
            break;
         dircnt++;

         // 剔除代表目前資料夾的 '.'
         if (fs2.name[0] != '.' && (fs2.attrib & FA_DIREC))
         {  // 在指定的來源路徑尾端串接找到的資料夾名稱建立新的來源路徑
            _splitpath(inDir, drive, dir, name, ext);
            strcat(dir, fs2.name);
            // 建立新的來源路徑
            _makepath(inDir, drive, dir, name, ext);
            // 在指定的輸出路徑尾端串接找到的資料夾名稱建立新的輸出路徑
            strcat(outDir, "\\");
            strcat(outDir, fs2.name);
            // printf("\nsys: in %s,  out %s", inDir, outDir);
            CheckDir(outDir); // 若輸出路徑不存在就先建立
            Process();        // 遞迴處理目前資料夾
            CutDir();         // 將來原及輸出路徑還原
         }
      }
   }
   return;
}

void CutDir()
{
   int i;
   _splitpath(inDir, drive, dir, name, ext);
   dir[strlen(dir) - 1] = 0;
   for (i = strlen(dir) - 1; i > 0; i--)
      if (dir[i] == '\\')
         break;
   dir[i] = 0;
   _makepath(inDir, drive, dir, name, ext);

   for (i = strlen(outDir) - 1; i > 0; i--)
      if (outDir[i] == '\\')
      {
         outDir[i] = 0;
         break;
      }
}

void CheckDir(byte *dir)
{
   if (dir[strlen(dir) - 1] == ':')
      return;
   if (access(dir, 0) != 0) // directory not exist
      if (mkdir(dir) != 0)
      {
         printf("%s create error!", dir);
         error("");
      }
   return;
}

void error(char *msg)
{
   printf("\n%s\x07\n", msg);
   getch();
   exit(1);
}

int getyna(void)
{
   int key;
   // while(bioskey(1)) bioskey(0);
   while (1)
   {
      key = getch() | 0x20;
      if (key == 'y' || key == 'n' || key == 'a')
         break;
   }
   return key;
}

void MakeName(byte *from, byte *tm, byte *name)
{
   int t, n, pos, len;

   len = strlen(tm);
   for (t = 0, n = 0; t < len; t++, n++)
   {
      if (tm[t] != '\\')
         name[n] = tm[t];
      else
      {
         pos = tm[++t] - '0';
         name[n] = from[pos];
      }
   }
}

//******************* Conv *****************************

// #include <stdio.h>
#include <mem.h>

// static FILE *ifp, *ofp;
static char str[4 * 1024], prestr[4 * 1024], tmpstr[4 * 1024];
extern char story, version[];
static char newpara,      // 這一行之前是空白行, 將這行視為新段落的第一行
            nextnewpara,  // 這一行之前雖然不是空白行, 但因為前一行是表格內分界線之類, 
                          // 仍將此行視為新段落的第一行
            skipline,     // 是否要略過這一行 (例如表格的分隔線) 不輸出 
            newblock;     // 進入 #框 或是 #表 
static int line;
static char bold, *outFileName;
static int prespace; // 非中文字開頭的行前是否要加空白

static char stack[21], sp; // p,f,t,T,B,P,I,N
// 段落標記: #圖(f), !!!(p), ┌(t)
// 區塊標記: #項(I), #程(P), #表(T), #框(B), #習(Q)
//      new- #步(S), #甲(W), #乙(X), #丙(Y), #丁(Z), (甲乙-正文加粗, 步丙丁-筆圖加粗)
// 其他    : 正文(N)

// void Convert(char *inFile, char *outFile);
void Conv1stLine(char *p);
void ConvBold_Space(char *p);
void ConvTable(char *p);
void ConvBox(char *p);
void ConvBlockEnd(void/*char *p*/);
void converror(void);
void OpenFiles(char *ifname, char *ofname);
void CloseFiles(void);
int ReadLine(void);
int Equal(char *str, char *cmp);
void AddStyle(char *s);
void LeftTrim(char *p);
void RightTrim(char *p);
void CheckBlockEnd(char *str);

char push(char c);
char pop(void);
char fetch(void);
char check(char c);
int getyn(void);

void Convert(char *inFile, char *outFile)
{
   int i, j, first;
   char *p;
   char *pBox1, *pBox2, *pTable;
   int ismarker;
   //   pBox1 = "□";
   //   pBox2 = "○";
   //   pTable = "┌";
   outFileName = outFile;
   OpenFiles(inFile, outFile);
   // AddStyle("<PMTags Win 1.0>");
   AddStyle("<_正文>Tx.exe ");
   AddStyle(version);
   AddStyle(" 版\n");
   stack[0] = 'N';
   first = 1;
   line = 0;
   while (ReadLine())
   {
      p = str;

      // 新段落或全文的第1行
      if (newpara || first || nextnewpara)
      {
         if (bold) // 若粗體未結束
         {
            printf("\nError at line %d: Bold mark can't across paragraf.\n%s", line, p);
            converror();
         }
         prespace = 0; // 非中文字開頭的行前不加空白
         fputs("\n", ofp);
         switch (fetch()) // 清理段落標記
         {
         case 'f':           // 圖說
         case 'p':           // 筆圖 
         case 't':           // 表格
            pop();
            break;
         }
         nextnewpara = 0;
         while (*p == ' ')
            p++;
         if (p[0] == '[')
         {
            i = strlen(p);
            ismarker = 1;
            if (p[i - 2] == ']')
            {
               p[i - 2] = '\n';
               p[i - 1] = 0;
            }
            else
            {
               printf("\n□ Is line %d Middle Mark (第 %d 行是中標嗎) ? "
                      "(Y/N)\n%s\x07",
                      line, line, p);
               if (getyn() == 'n')
               {
                  Conv1stLine(p);
                  ismarker = 0;
               }
            }
            if (ismarker)
            {
               AddStyle("<2中標>");
               p += 1;
               CheckBlockEnd("中標");
            }
         }
         else if (p[0] == '<')
         {
            if (p[1] != '<')
            {
               Conv1stLine(p);
            }
            else
            {
               AddStyle("<3小標>");
               p += 2;
               i = strlen(p);
               if (p[i - 3] == '>')
               {
                  p[i - 3] = '\n';
                  p[i - 2] = 0;
               }
               CheckBlockEnd("小標");
            }
         }
         else if (p[0] == '!')
         {
            AddStyle("<6筆圖>");
            push('p');
         }
         else if (p[0] == '#')
         {
            if (Equal(p + 1, "圖"))
            {
               AddStyle("<7圖說>");
               push('f');
            }
            else if (Equal(p + 1, "程"))
            {
               AddStyle("<6程式>");
               push('P');
            }
            else
            {
               nextnewpara = 1;
               if (Equal(p + 1, "項"))
               {
                  AddStyle("<4□-1>");
                  push('I');
               }
               else if (Equal(p + 1, "習"))
               {
                  AddStyle("<9習題>");
                  push('Q');
               }
               else if (Equal(p + 1, "步"))
               {
                  AddStyle("<9步驟>");
                  push('S');
               }
               else if (Equal(p + 1, "甲"))
               {
                  AddStyle("<9甲>");
                  push('W');
               }
               else if (Equal(p + 1, "乙"))
               {
                  AddStyle("<9乙>");
                  push('X');
               }
               else if (Equal(p + 1, "丙"))
               {
                  AddStyle("<9丙>");
                  push('Y');
               }
               else if (Equal(p + 1, "丁"))
               {
                  AddStyle("<9丁>");
                  push('Z');
               }
               else if (Equal(p + 1, "框"))
               {
                  AddStyle("<5目的>");
                  push('B');
                  /* skipline = */ newblock = 1;
               }
               else if (Equal(p + 1, "表"))
               {
                  AddStyle("<_正文>");
                  push('T');
                  newblock = 1;
                  p[strlen(p) - 1] = 0;
               }
               else if (Equal(p + 1, "##"))
               {
                  ConvBlockEnd(/*p*/);
               }
               else
               {
                  printf("\nError at line %d: '#' not allowed.\n%s",
                         line, p);
                  converror();
               }
            }
         }
         else if (memcmp(p, "□", 2) == 0 || memcmp(p, "○", 2) == 0)
         {
            if (fetch() == 'I') // □ or ○ 項目
               AddStyle("<4□>");
            else
               Conv1stLine(p);
         }
         else if (memcmp(p, "┌", 2) == 0) // ┌ 表格或程式(方框)
         {
            push('t');
            ConvTable(p);
         }
         else
         {
            ismarker = 0;
            // 數字開頭或是 A-數字 這樣格式開頭
            if (isdigit(p[0]) || (isalpha(p[0]) && p[1] == '-' && isdigit(p[2])/*isalpha(p[2])*/))
            {
               if (p[1] == '-' || p[2] == '-') // "數字或是字母-數字" 的格式是節名
               {
                  CheckBlockEnd("節名");
                  AddStyle("<1節名>");
                  ismarker = 1;
               }
               else if (fetch() == 'I' && (p[1] == '.' || p[2] == '.')) // "數字." 這樣的格式是編號條列項目
               {
                  AddStyle("<4□>");
                  ismarker = 1;
               }
            }
            if (ismarker == 0) // 不是節名或是編號項目
               Conv1stLine(p); // 以段落第一行處理
         }
      }
      // 處理段落第2.3.4...行, 要檢查區塊標記的結尾 (### 及 └),
      // 若為不併行段落 (圖說、表格、程式) 則要加排式
      else
      {
         if (Equal(p, "###"))
         {
            fputs("\n", ofp); // 換行, 否則 ### 會併到上一行行尾
            ConvBlockEnd(/*p*/);
         }
         else
            switch (fetch())
            {
            case 'f':
               AddStyle("<7圖說>");
               break;
            case 't':
            case 'T':
               ConvTable(p);
               break;
            case 'P':
               AddStyle("<6程式>");
               break;
            }
      }

      // 處理行內文字
      switch (fetch())
      {
      case 'f':
      case 'P':
      case 'T':
      case 't':
         break;
      default:
         if (p[strlen(p) - 1] == '\n')
            p[strlen(p) - 1] = 0; // 去換行
         break;
      }
      if (!skipline)
      {
         ConvBold_Space(p);
         fputs(p, ofp);
      }
      skipline = 0;
      first = 0;
   }
   CheckBlockEnd("檔尾");
   CloseFiles();
}

// 處理無標記段落的第1行, 若在區塊內則為正文
void Conv1stLine(char *p)
{
   switch (fetch())
   {
   case 'I':
      AddStyle("<4□-1>");
      break;
   case 'Q':
      AddStyle("<9習題>");
      break;
   case 'S':
      AddStyle("<9步驟>");
      break;
   case 'W':
      AddStyle("<9甲>");
      break;
   case 'X':
      AddStyle("<9乙>");
      break;
   case 'Y':
      AddStyle("<9丙>");
      break;
   case 'Z':
      AddStyle("<9丁>");
      break;
   case 'B':
      ConvBox(p);
      break;
   case 'T':
   case 't':
      ConvTable(p);
      break;
   case 'P':
      AddStyle("<6程式>");
      break;
   case 'N':
      AddStyle("<_正文>");
      break;
   }
}

void ConvBold_Space(char *p)
{
   int i, j, len, chin;
   char *q, *r, b, cr = 0; // cr <F4>之前沒有設定初值, 
                           // 導致圖說、程式都會額外加上換行

   q = tmpstr;
   strcpy(q, p);

   if (fetch() != 'f' && fetch() != 'P')
   {
      LeftTrim(q);

      // 先移除換行符號
      len = strlen(q);
      if (q[len - 1] == '\n')
      {
         cr = 1;
         q[len - 1] = 0;
      }
      else
         cr = 0;

      // 檢查前後是否要加空白
      if (prespace && *q > 0)
         *(p++) = ' ';
      prespace = 1;
      len = strlen(q);
      for (i = 0; i < len; i++)
      {
         if (q[i] < 0)
         {
            i++;
            chin = 1;
         }
         else
            chin = 0;
      }
      if (!chin && q[len - 1] != ' ')
      {
         q[len] = ' ';
         q[len + 1] = 0;
         prespace = 0;
      }
   }

   // 處理加粗
   r = q;
   // b = (fetch() == 'N' || fetch() == 'I' || fetch() == 'Q' ? 'n': 'f');
   b = (strchr("NIQWX", fetch()) ? 'n' : 'f');
   if (check('B'))
      b = 'f'; // if is inside a box
   while (*q)
   {
   recheck:
      if (Equal(q, "【") || Equal(q, "〈"))
      {
         if (bold)
         {
            printf("\nError at line %d: Bold mark can't appear in bold area (粗體/斜體區域中不可有【或〈.)\n%s", line, r);
            converror();
         }
         bold = Equal(q, "【") ? b : 'i';
         q += 2;
         goto recheck;
      }
      if (Equal(q, "】") || Equal(q, "〉"))
      {
         if (!bold)
         {
            printf("\nError at line %d:Bold mark not match ( 】、〉前沒有【、〈.)\n%s", line, r);
            converror();
         }
         bold = 0;
         q += 2;
         goto recheck;
      }
      if (*q != 0)
      {
         if (bold)
         {
            strcpy(p, (bold == 'n' ? "【" : (bold == 'f' ? "《" : "〈")));
            p += 2;
         }
         *(p++) = *(q++);
         if (*(q - 1) < 0)
            *(p++) = *(q++); // 中文字
         if (bold)
         {
            strcpy(p, (bold == 'n' ? "】" : (bold == 'f' ? "》" : "〉")));
            p += 2;
         }
      }
   }
   if (cr)
      *(p++) = '\n';
   *p = 0;
}

void ConvBox(char *p)
{
   if (Equal(p, "─") || Equal(p, "---"))
   {
      // if (newblock)
      // nextnewpara = 1;
      newblock = 0;
      nextnewpara = 1;
      AddStyle("<5目的標>");
   }
   else
      AddStyle("<5目的>");
}

void ConvTable(char *p)
{
   int i, j, k, len;

   if (fetch() == 'T') // 區塊表格
   {
      if (Equal(p, "─") || Equal(p, "---"))
      {
         skipline = 1;
         if (!newblock) {
            fputs("\n", ofp);
         }
         newblock = 0;
      }
      else
      {
         for (i = 0; p[i] != 0; i++)
         {
            if (Equal(p + i, "   "))
            {
               p[i] = '\t';
               j = i + 3;
               while (p[j] == ' ')
                  j++;
               for (k = i + 1; p[j] != 0; k++, j++)
                  p[k] = p[j];
               p[k] = 0;
            }
            if (p[i + 1] == '.' && Equal(p + i + 2, "   "))
               p[i + 1] = ' ';
         }
         p[i - 1] = 0; // 移除換行符號
         if (Equal(prestr, "─") || Equal(prestr, "---"))
            AddStyle("<8表格>");
         else
            fputs("︼", ofp);
      }
   }
   else // 手繪表格
   {
      LeftTrim(p);
      LeftTrim(prestr);
      if (Equal(p, "┌"))
      {
         AddStyle("<_正文>");
      }
      else if (Equal(p, "└"))
      {
         fputs("\n", ofp);
         AddStyle("<_正文>");
         pop();
         nextnewpara = 1;
      }
      else if (Equal(p, "├"))
      {
         skipline = 1;
         fputs("\n", ofp);
      }
      else if (Equal(p, "│"))
      {
         j = 2;
         while (p[j] == ' ')
            j++;
         for (k = 0; p[j] != 0; k++, j++)
            p[k] = p[j];
         if (!Equal(p + k - 3, "│") && !Equal(p + k - 3, "┤")) // -3 是包含換行符號
         {
            printf("\nError at line %d: Can't process freehand table (手繪表格無法處理)\n", line);
            converror();
         }
         p[k - 3] = 0;
         k = k - 4;
         while (p[k] == ' ')
            p[k--] = 0;
         for (i = 0; p[i] != 0; i++)
         {
            if (Equal(p + i, "│") || Equal(p + i, "├") || Equal(p + i, "┤") || Equal(p + i, "┼"))
            {
               k = i - 1;
               while (p[k] == ' ')
                  k--;
               j = i + 2;
               while (p[j] == ' ')
                  j++;
               p[k + 1] = '\t';
               i = k + 1;
               for (k += 2; p[j] != 0; k++, j++)
                  p[k] = p[j];
               p[k] = 0;
            }
         }
         if (Equal(prestr, "├") || Equal(prestr, "┌"))
            AddStyle("<8表格>");
         else
            fputs("︼", ofp);
      }
      else
      {
         printf("\nError at line %d: Can't process freehand table (手繪表格無法處理)\n", line);
         converror();
      }
   }
}

// 處理 ### 之段落標記結束
void ConvBlockEnd(void/*char *p*/)
{
   if (fetch() == 'I' && !newpara) {
      fputs("\n", ofp);
   }
redo:
   switch (pop())
   {
   case 'I':
      AddStyle("<4□-1>");
      break;
   case 'Q':
      AddStyle("<9習題>");
      break;
   case 'S':
      AddStyle("<9步驟>");
      break;
   case 'W':
      AddStyle("<9甲>");
      break;
   case 'X':
      AddStyle("<9乙>");
      break;
   case 'Y':
      AddStyle("<9丙>");
      break;
   case 'Z':
      AddStyle("<9丁>");
      break;
   case 'B':
      AddStyle("<5目的>");
      break;
   case 'T':
      AddStyle("<_正文>");
      break;
   case 'P':
      AddStyle("<6程式>");
      break;
   case 'f':
      printf("\nWarnning at line %d: 圖說後未空行\x07", line);
      getch();
      goto redo;
   case 't':
      printf("\nWarnning at line %d: 手繪表格後未空行\x07", line);
      getch();
      goto redo;
   case 'p':
      printf("\nWarnning at line %d: 筆圖後未空行\x07", line);
      getch();
      goto redo;
   default:
      printf("\nError: ### not match at line %d.\n", line);
      converror();
   }
}

void OpenFiles(char *ifname, char *ofname)
{
   int i, j;
   if ((ifp = fopen(ifname, "rt")) == 0L)
   {
      printf("\n%s open error!\n", ifname);
      converror();
   }
   if ((ofp = fopen(ofname, "wt")) == 0L)
   {
      printf("\n%s open error!\n", ofname);
      converror();
   }
   // 設定暫存區大小
   i = setvbuf(ifp, 0L, _IOFBF, 16 * 1024);
   j = setvbuf(ofp, 0L, _IOFBF, 16 * 1024);
   if (i || j)
   {
      printf("\nMemory is not enough!\n");
      converror();
   }
}

void CloseFiles(void)
{
   fclose(ifp);
   fclose(ofp);
}

// 獨取下一個空白行並且刪除開頭的空白字元
int ReadLine(void)
{
   int i;
   char *z;

   strcpy(prestr, str);
   z = str;
   // 讀取下一個非空白行
   for (i = 0, str[0] = '\n'; z[0] == '\n'; i++)
   {
      line++;
      if (fgets(str, 8 * 1024, ifp) == 0L)
         return 0;
      if (fetch() == 'P')      // 程式碼要保留開頭的空白字元
         break;
      // LeftTrim(str);
      while (*z == ' ')        // 移除一般文字段落的開頭空白字元
         z++;
   }
   newpara = (i > 1 ? 1 : 0);  // 有略過空白行才讀到內文表示是新的段落
   RightTrim(str);             // 刪除行尾的空白
   return 1;
}

// 字串比較, 應該可以改用 strcmp
int Equal(char *str, char *cmp)
{
   while (*cmp)
   {
      if (*str != *cmp)
         return 0;
      cmp++;
      str++;
   }
   return 1;
}

// 添加樣式
void AddStyle(char *s)
{
   // 在框及習題中不加項目樣式
   if (Equal(s, "<4□") && (check('B') || check('Q')))
      s = "<5目的>";
   if (story) {      // 附加樣式
      fputs(s, ofp);
   }
}

//*****************************************

char push(char c)
{
   stack[++sp] = c;
   return sp;
}

char pop(void)
{
   if (sp == 0)
   {
      printf("\nError: Block-Marker not match at line %d.\n", line);
      converror();
   }
   return stack[sp--];
}

char fetch(void)
{
   if (sp < 0)
      return 0;
   else
      return stack[sp];
}

char check(char c)
{
   char i;
   for (i = sp; i > 0; i--)
      if (stack[i] == c)
         break;
   return i;
}

void LeftTrim(char *p)
{
   int j, k;
   j = 0;
   while (p[j] == ' ')
      j++;
   if (j > 0)
   {
      for (k = 0; p[j] != 0; k++, j++)
         p[k] = p[j];
      p[k] = 0;
   }
}

void RightTrim(char *p)
{
   int i, j, len;
   len = strlen(p);
   if (p[len - 1] == '\n')
      j = len - 2;
   else
      j = len - 1;
   for (i = j; i > 0; i--)
   {
      if (p[i] != ' ')
         break;
   }
   if (j == len - 2)
   {
      p[i + 1] = '\n';
      p[i + 2] = 0;
   }
   else
      p[i + 1] = 0;
}

// 檢查在 str 指定的章、節、中標、小標前是否有尚未結束的區塊
void CheckBlockEnd(char *str)
{
   char c, *msg;
recheck:
   c = fetch();
   if (c != 'N') // 區塊標記未結束
   {
      switch (c)
      {
      case 'I':
         msg = "#項";
         break;
      case 'B':
         msg = "#框";
         break;
      case 'T':
         msg = "#表";
         break;
      case 'P':
         msg = "#程";
         break;
      case 'Q':
         msg = "#習";
         break;
      case 'S':
         msg = "#步";
         break;
      case 'W':
         msg = "#甲";
         break;
      case 'X':
         msg = "#乙";
         break;
      case 'Y':
         msg = "#丙";
         break;
      case 'Z':
         msg = "#丁";
         break;
      default:
         pop();
         goto recheck;
      }

      printf("\nError at line %d: <%s> must be ended before %s.\n", line, msg, str);
      converror();
   }
}

void converror(void)
{
   printf("\x07\n");
   CloseFiles();
   remove(outFileName);
   getch();
   exit(1);
}

int getyn(void)
{
   int key;
   // while(bioskey(1)) bioskey(0);
   while (1)
   {
      key = getch() | 0x20;
      if (key == 'y' || key == 'n')
         break;
   }
   return key;
}
