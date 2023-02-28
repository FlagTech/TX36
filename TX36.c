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
char sys,           // �O�_�n�B�z�l��Ƨ�
     over,          // �O�_�n�л\�w�g�s�b����X��
     story = 1;     // �O�_�n�a��X���C�@��e���[�W�˦��W��
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
   printf("\n++++++++++++++++++++++++++++++++++++++������+++++++++++++++"
          "\nKen's text tool for PageMaker 6.xC                 ver %s"
          "\nTx (C) CopyRight 1997-2003                         by Ken L"
          "\n+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n",
          version);
   if (argc > 1 && (argv[1][0] == '/' || argv[1][0] == '!')) // �����w�Ѹ�
   {
      if (argv[1][1] != '/')             // tx /FXXXX
         strcpy(Template, argv[1] + 1);
      else                               // tx //FXXXX
      {
         sys = 1;                        // �]�t�l��Ƨ�
         strcpy(Template, argv[1] + 2);
      }
      if (argv[1][0] == '!')
         story = 0;
      if (strlen(Template) > 6)          // �Ѹ��Ӫ� 
         help();
      if (strlen(Template) < 6)
         strcat(Template, "-");
      strcat(Template, "\\2\\3.txt");    // ��X�ɦW�� "�Ѹ�-����.txt" ���榡
      strcpy(outDir, ".");               // �w�]��X��Ƨ����ثe��Ƨ�
      if (argc == 2)                     // tx /FXXXX �����w�ɦW
      {
         strcpy(inDir, "ch??.txt");      // �j�M�Ҧ� "chxx" ���ɦW
         Process();
      }
      else                               // tx /FXXXX chxx.txt ....
      {
         for (i = 2; i < argc; i++)      // �@�@�B�z�R�O�C�����w���ɦW
         {
            strcpy(inDir, argv[i]);
            Process();
         }
      }
   }
   else                                      // <F4>�S�����w�Ѹ�
   {
      for (idx = argc - 1; idx > 0; idx--)   // �q�R�O�C���ݩ��������w��X���|���Ѽ�
         if (argv[idx][0] != '/')
            break;
      for (i = idx - 1; i > 0; i--)          // �q��X���|�A���^��̫�@�ӿ���|���Ѽ�
         if (argv[i][0] != '/')
            break;
      if (idx == 0 || i == 0)                // �S�����w��X���|�άO��J���|
         help();                             // �Ѽƿ��~, ��ܻ������ϥΪ̰Ѧ�

      for (i = argc - 1; i > 0; i--)         // �q�R�O����ݩ��^��X��X�ﶵ
         if (argv[i][0] == '/')
            switch (argv[i][1] | 0x20)       // ��j�p�r���ܤp�g 0x65 | 0x20 => 0x45
            {
            case 's':                        // �]�t�l��Ƨ�
               sys = 1;
               break;
            case 'n':                        // ���w�ɦW�˪�
               strcpy(Template, argv[i] + 2);
               break;
            case 'o':
               over = 1;                     // �л\�w�s�b����X��
               break;
            case 'p':
               story = 0;                    // �����[�˦�
               break;
            default:
               help();
            }

      strcpy(outDir, argv[idx]);             // �]�w��X���|
      i = strlen(outDir) - 1;                // �������|���ݪ� '\'
      if (outDir[i] == '\\')
         outDir[i] = 0;
      CheckDir(outDir);                      // �ˬd��X���|, �Y���s�b�N�إ߸Ӹ�Ƨ�
      for (j = 1; j < idx; j++)              // �@�@��X�ӭ���|
      {
         if (argv[j][0] != '/')
         {
            strcpy(inDir, argv[j]);
            Process();                       // �B�z�ثe�o�@�Өӷ����|
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
      // �Χ�쪺�ɦW���N�i��t���U�Φr�����ӷ��ɦW
      _splitpath(inDir, drive, dir, name, ext);
      _makepath(inPath, drive, dir, fs.name, "");

      // �إ߿�X�ɸ��|
      if (Template[0] == 0)       // �S�����w�Ѹ��ɤ��|���˪�
      {
         strcpy(name, fs.name);   // �Ψӷ��ɦW�r���� 'x' ����X�ɦW
         name[0] = 'x';
      }
      else // �Χ�쪺�ɦW�����r���������˪����� \x ����r��
         MakeName(fs.name, Template, name);
      ext[0] = 0;
      _makepath(outPath, "", outDir, name, ext); // �إ߿�X�ɦW

      // �����w�л\��X�ɦӥB��X�ɤw�g�s�b
      if ((over == 0) && (ofp = fopen(outPath, "rb")) != NULL)
      {  // �߰ݬO�_�n�л\��X��
         printf("\n%s already exist! overwrite? (Yes/No/All)", outPath);
         i = getyna(); // ���ϥΪ̫������@�k
         if (i == 'n')
            goto skip;
         if (i == 'a')
            over = 1;
      }

      cnt++;

      // ��ܥثe�n�B�z���ӷ��ɻP��X�ɦW
      printf("\n%s ==> %s", inPath, outPath);
      Convert(inPath, outPath);   // �B�z�ثe�ɮ�
   skip:
      done = _dos_findnext(&fs);  // ��U�@����
   }

   if (sys)                       // �p�G�n�]�t�l��Ƨ�
   {
      byte dircnt, i;
      dircnt = 0;
      // printf("\nsys: in %s,  out %s", inDir, outDir);
      while (1)
      {
         // �զX�X "�ثe�����|\*.*:find directory
         _splitpath(inDir, drive, dir, name, ext);
         _makepath(inPath, drive, dir, "*", "*");
         // printf("\ninpth dir %s", inPath);
         // �u�j�M��Ƨ�
         done = _dos_findfirst(inPath, FA_DIREC, &fs2);
         for (i = 0; i < dircnt; i++)
            done = _dos_findnext(&fs2);
         if (done)
            break;
         dircnt++;

         // �簣�N��ثe��Ƨ��� '.'
         if (fs2.name[0] != '.' && (fs2.attrib & FA_DIREC))
         {  // �b���w���ӷ����|���ݦ걵��쪺��Ƨ��W�٫إ߷s���ӷ����|
            _splitpath(inDir, drive, dir, name, ext);
            strcat(dir, fs2.name);
            // �إ߷s���ӷ����|
            _makepath(inDir, drive, dir, name, ext);
            // �b���w����X���|���ݦ걵��쪺��Ƨ��W�٫إ߷s����X���|
            strcat(outDir, "\\");
            strcat(outDir, fs2.name);
            // printf("\nsys: in %s,  out %s", inDir, outDir);
            CheckDir(outDir); // �Y��X���|���s�b�N���إ�
            Process();        // ���j�B�z�ثe��Ƨ�
            CutDir();         // �N�ӭ�ο�X���|�٭�
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
static char newpara,      // �o�@�椧�e�O�ťզ�, �N�o������s�q�����Ĥ@��
            nextnewpara,  // �o�@�椧�e���M���O�ťզ�, ���]���e�@��O��椺���ɽu����, 
                          // ���N��������s�q�����Ĥ@��
            skipline,     // �O�_�n���L�o�@�� (�Ҧp��檺���j�u) ����X 
            newblock;     // �i�J #�� �άO #�� 
static int line;
static char bold, *outFileName;
static int prespace; // �D����r�}�Y����e�O�_�n�[�ť�

static char stack[21], sp; // p,f,t,T,B,P,I,N
// �q���аO: #��(f), !!!(p), �z(t)
// �϶��аO: #��(I), #�{(P), #��(T), #��(B), #��(Q)
//      new- #�B(S), #��(W), #�A(X), #��(Y), #�B(Z), (�ҤA-����[��, �B���B-���ϥ[��)
// ��L    : ����(N)

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
   //   pBox1 = "��";
   //   pBox2 = "��";
   //   pTable = "�z";
   outFileName = outFile;
   OpenFiles(inFile, outFile);
   // AddStyle("<PMTags Win 1.0>");
   AddStyle("<_����>Tx.exe ");
   AddStyle(version);
   AddStyle(" ��\n");
   stack[0] = 'N';
   first = 1;
   line = 0;
   while (ReadLine())
   {
      p = str;

      // �s�q���Υ��媺��1��
      if (newpara || first || nextnewpara)
      {
         if (bold) // �Y���饼����
         {
            printf("\nError at line %d: Bold mark can't across paragraf.\n%s", line, p);
            converror();
         }
         prespace = 0; // �D����r�}�Y����e���[�ť�
         fputs("\n", ofp);
         switch (fetch()) // �M�z�q���аO
         {
         case 'f':           // �ϻ�
         case 'p':           // ���� 
         case 't':           // ���
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
               printf("\n�� Is line %d Middle Mark (�� %d ��O���ж�) ? "
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
               AddStyle("<2����>");
               p += 1;
               CheckBlockEnd("����");
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
               AddStyle("<3�p��>");
               p += 2;
               i = strlen(p);
               if (p[i - 3] == '>')
               {
                  p[i - 3] = '\n';
                  p[i - 2] = 0;
               }
               CheckBlockEnd("�p��");
            }
         }
         else if (p[0] == '!')
         {
            AddStyle("<6����>");
            push('p');
         }
         else if (p[0] == '#')
         {
            if (Equal(p + 1, "��"))
            {
               AddStyle("<7�ϻ�>");
               push('f');
            }
            else if (Equal(p + 1, "�{"))
            {
               AddStyle("<6�{��>");
               push('P');
            }
            else
            {
               nextnewpara = 1;
               if (Equal(p + 1, "��"))
               {
                  AddStyle("<4��-1>");
                  push('I');
               }
               else if (Equal(p + 1, "��"))
               {
                  AddStyle("<9���D>");
                  push('Q');
               }
               else if (Equal(p + 1, "�B"))
               {
                  AddStyle("<9�B�J>");
                  push('S');
               }
               else if (Equal(p + 1, "��"))
               {
                  AddStyle("<9��>");
                  push('W');
               }
               else if (Equal(p + 1, "�A"))
               {
                  AddStyle("<9�A>");
                  push('X');
               }
               else if (Equal(p + 1, "��"))
               {
                  AddStyle("<9��>");
                  push('Y');
               }
               else if (Equal(p + 1, "�B"))
               {
                  AddStyle("<9�B>");
                  push('Z');
               }
               else if (Equal(p + 1, "��"))
               {
                  AddStyle("<5�ت�>");
                  push('B');
                  /* skipline = */ newblock = 1;
               }
               else if (Equal(p + 1, "��"))
               {
                  AddStyle("<_����>");
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
         else if (memcmp(p, "��", 2) == 0 || memcmp(p, "��", 2) == 0)
         {
            if (fetch() == 'I') // �� or �� ����
               AddStyle("<4��>");
            else
               Conv1stLine(p);
         }
         else if (memcmp(p, "�z", 2) == 0) // �z ���ε{��(���)
         {
            push('t');
            ConvTable(p);
         }
         else
         {
            ismarker = 0;
            // �Ʀr�}�Y�άO A-�Ʀr �o�ˮ榡�}�Y
            if (isdigit(p[0]) || (isalpha(p[0]) && p[1] == '-' && isdigit(p[2])/*isalpha(p[2])*/))
            {
               if (p[1] == '-' || p[2] == '-') // "�Ʀr�άO�r��-�Ʀr" ���榡�O�`�W
               {
                  CheckBlockEnd("�`�W");
                  AddStyle("<1�`�W>");
                  ismarker = 1;
               }
               else if (fetch() == 'I' && (p[1] == '.' || p[2] == '.')) // "�Ʀr." �o�˪��榡�O�s�����C����
               {
                  AddStyle("<4��>");
                  ismarker = 1;
               }
            }
            if (ismarker == 0) // ���O�`�W�άO�s������
               Conv1stLine(p); // �H�q���Ĥ@��B�z
         }
      }
      // �B�z�q����2.3.4...��, �n�ˬd�϶��аO������ (### �� �|),
      // �Y�����֦�q�� (�ϻ��B���B�{��) �h�n�[�Ʀ�
      else
      {
         if (Equal(p, "###"))
         {
            fputs("\n", ofp); // ����, �_�h ### �|�֨�W�@����
            ConvBlockEnd(/*p*/);
         }
         else
            switch (fetch())
            {
            case 'f':
               AddStyle("<7�ϻ�>");
               break;
            case 't':
            case 'T':
               ConvTable(p);
               break;
            case 'P':
               AddStyle("<6�{��>");
               break;
            }
      }

      // �B�z�椺��r
      switch (fetch())
      {
      case 'f':
      case 'P':
      case 'T':
      case 't':
         break;
      default:
         if (p[strlen(p) - 1] == '\n')
            p[strlen(p) - 1] = 0; // �h����
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
   CheckBlockEnd("�ɧ�");
   CloseFiles();
}

// �B�z�L�аO�q������1��, �Y�b�϶����h������
void Conv1stLine(char *p)
{
   switch (fetch())
   {
   case 'I':
      AddStyle("<4��-1>");
      break;
   case 'Q':
      AddStyle("<9���D>");
      break;
   case 'S':
      AddStyle("<9�B�J>");
      break;
   case 'W':
      AddStyle("<9��>");
      break;
   case 'X':
      AddStyle("<9�A>");
      break;
   case 'Y':
      AddStyle("<9��>");
      break;
   case 'Z':
      AddStyle("<9�B>");
      break;
   case 'B':
      ConvBox(p);
      break;
   case 'T':
   case 't':
      ConvTable(p);
      break;
   case 'P':
      AddStyle("<6�{��>");
      break;
   case 'N':
      AddStyle("<_����>");
      break;
   }
}

void ConvBold_Space(char *p)
{
   int i, j, len, chin;
   char *q, *r, b, cr = 0; // cr <F4>���e�S���]�w���, 
                           // �ɭP�ϻ��B�{�����|�B�~�[�W����

   q = tmpstr;
   strcpy(q, p);

   if (fetch() != 'f' && fetch() != 'P')
   {
      LeftTrim(q);

      // ����������Ÿ�
      len = strlen(q);
      if (q[len - 1] == '\n')
      {
         cr = 1;
         q[len - 1] = 0;
      }
      else
         cr = 0;

      // �ˬd�e��O�_�n�[�ť�
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

   // �B�z�[��
   r = q;
   // b = (fetch() == 'N' || fetch() == 'I' || fetch() == 'Q' ? 'n': 'f');
   b = (strchr("NIQWX", fetch()) ? 'n' : 'f');
   if (check('B'))
      b = 'f'; // if is inside a box
   while (*q)
   {
   recheck:
      if (Equal(q, "�i") || Equal(q, "�q"))
      {
         if (bold)
         {
            printf("\nError at line %d: Bold mark can't appear in bold area (����/����ϰ줤���i���i�Ρq.)\n%s", line, r);
            converror();
         }
         bold = Equal(q, "�i") ? b : 'i';
         q += 2;
         goto recheck;
      }
      if (Equal(q, "�j") || Equal(q, "�r"))
      {
         if (!bold)
         {
            printf("\nError at line %d:Bold mark not match ( �j�B�r�e�S���i�B�q.)\n%s", line, r);
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
            strcpy(p, (bold == 'n' ? "�i" : (bold == 'f' ? "�m" : "�q")));
            p += 2;
         }
         *(p++) = *(q++);
         if (*(q - 1) < 0)
            *(p++) = *(q++); // ����r
         if (bold)
         {
            strcpy(p, (bold == 'n' ? "�j" : (bold == 'f' ? "�n" : "�r")));
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
   if (Equal(p, "�w") || Equal(p, "---"))
   {
      // if (newblock)
      // nextnewpara = 1;
      newblock = 0;
      nextnewpara = 1;
      AddStyle("<5�ت���>");
   }
   else
      AddStyle("<5�ت�>");
}

void ConvTable(char *p)
{
   int i, j, k, len;

   if (fetch() == 'T') // �϶����
   {
      if (Equal(p, "�w") || Equal(p, "---"))
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
         p[i - 1] = 0; // ��������Ÿ�
         if (Equal(prestr, "�w") || Equal(prestr, "---"))
            AddStyle("<8���>");
         else
            fputs("�l", ofp);
      }
   }
   else // ��ø���
   {
      LeftTrim(p);
      LeftTrim(prestr);
      if (Equal(p, "�z"))
      {
         AddStyle("<_����>");
      }
      else if (Equal(p, "�|"))
      {
         fputs("\n", ofp);
         AddStyle("<_����>");
         pop();
         nextnewpara = 1;
      }
      else if (Equal(p, "�u"))
      {
         skipline = 1;
         fputs("\n", ofp);
      }
      else if (Equal(p, "�x"))
      {
         j = 2;
         while (p[j] == ' ')
            j++;
         for (k = 0; p[j] != 0; k++, j++)
            p[k] = p[j];
         if (!Equal(p + k - 3, "�x") && !Equal(p + k - 3, "�t")) // -3 �O�]�t����Ÿ�
         {
            printf("\nError at line %d: Can't process freehand table (��ø���L�k�B�z)\n", line);
            converror();
         }
         p[k - 3] = 0;
         k = k - 4;
         while (p[k] == ' ')
            p[k--] = 0;
         for (i = 0; p[i] != 0; i++)
         {
            if (Equal(p + i, "�x") || Equal(p + i, "�u") || Equal(p + i, "�t") || Equal(p + i, "�q"))
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
         if (Equal(prestr, "�u") || Equal(prestr, "�z"))
            AddStyle("<8���>");
         else
            fputs("�l", ofp);
      }
      else
      {
         printf("\nError at line %d: Can't process freehand table (��ø���L�k�B�z)\n", line);
         converror();
      }
   }
}

// �B�z ### ���q���аO����
void ConvBlockEnd(void/*char *p*/)
{
   if (fetch() == 'I' && !newpara) {
      fputs("\n", ofp);
   }
redo:
   switch (pop())
   {
   case 'I':
      AddStyle("<4��-1>");
      break;
   case 'Q':
      AddStyle("<9���D>");
      break;
   case 'S':
      AddStyle("<9�B�J>");
      break;
   case 'W':
      AddStyle("<9��>");
      break;
   case 'X':
      AddStyle("<9�A>");
      break;
   case 'Y':
      AddStyle("<9��>");
      break;
   case 'Z':
      AddStyle("<9�B>");
      break;
   case 'B':
      AddStyle("<5�ت�>");
      break;
   case 'T':
      AddStyle("<_����>");
      break;
   case 'P':
      AddStyle("<6�{��>");
      break;
   case 'f':
      printf("\nWarnning at line %d: �ϻ��᥼�Ŧ�\x07", line);
      getch();
      goto redo;
   case 't':
      printf("\nWarnning at line %d: ��ø���᥼�Ŧ�\x07", line);
      getch();
      goto redo;
   case 'p':
      printf("\nWarnning at line %d: ���ϫ᥼�Ŧ�\x07", line);
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
   // �]�w�Ȧs�Ϥj�p
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

// �W���U�@�Ӫťզ�åB�R���}�Y���ťզr��
int ReadLine(void)
{
   int i;
   char *z;

   strcpy(prestr, str);
   z = str;
   // Ū���U�@�ӫD�ťզ�
   for (i = 0, str[0] = '\n'; z[0] == '\n'; i++)
   {
      line++;
      if (fgets(str, 8 * 1024, ifp) == 0L)
         return 0;
      if (fetch() == 'P')      // �{���X�n�O�d�}�Y���ťզr��
         break;
      // LeftTrim(str);
      while (*z == ' ')        // �����@���r�q�����}�Y�ťզr��
         z++;
   }
   newpara = (i > 1 ? 1 : 0);  // �����L�ťզ�~Ū�줺���ܬO�s���q��
   RightTrim(str);             // �R��������ť�
   return 1;
}

// �r����, ���ӥi�H��� strcmp
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

// �K�[�˦�
void AddStyle(char *s)
{
   // �b�ؤβ��D�����[���ؼ˦�
   if (Equal(s, "<4��") && (check('B') || check('Q')))
      s = "<5�ت�>";
   if (story) {      // ���[�˦�
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

// �ˬd�b str ���w�����B�`�B���СB�p�Ыe�O�_���|���������϶�
void CheckBlockEnd(char *str)
{
   char c, *msg;
recheck:
   c = fetch();
   if (c != 'N') // �϶��аO������
   {
      switch (c)
      {
      case 'I':
         msg = "#��";
         break;
      case 'B':
         msg = "#��";
         break;
      case 'T':
         msg = "#��";
         break;
      case 'P':
         msg = "#�{";
         break;
      case 'Q':
         msg = "#��";
         break;
      case 'S':
         msg = "#�B";
         break;
      case 'W':
         msg = "#��";
         break;
      case 'X':
         msg = "#�A";
         break;
      case 'Y':
         msg = "#��";
         break;
      case 'Z':
         msg = "#�B";
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
