#include<stdio.h>
#include<locale.h>
#include<wchar.h>
#include<stringapiset.h>

wchar_t buf[1000];
wchar_t fname[500];

int main(int argc, char **argv) {
  // 設定成 "zh_TW.UTF-8" 編碼
  char *loc = setlocale(LC_CTYPE, "cht");
  if (loc == NULL) {
    printf("字元編碼設置失敗～");
    return -1;
  }
  wprintf(L"zh_TW.UTF-8 編碼設置成功！\n");
  FILE *ifp, *ofp;
  MultiByteToWideChar(
      950, 
      MB_PRECOMPOSED,
      argv[1],
      -1,
      fname,
      500);
  wprintf(L"%d, %d, %lc, %ls\n", argc, wcslen(fname), fname[0], fname);
  ifp = _wfopen(fname, L"rt, ccs=UTF-8");
  ofp = _wfopen(L"test.txt", L"wt, ccs=UTF-8");
  while(fgetws(buf, 1000, ifp) != NULL) {
    int len = wcslen(buf);
    if(buf[len - 1] == L'\n') buf[len - 1] = L'\0';
    // _putws(buf);
    wprintf(L"%ls\n", buf);
    fputws(buf, ofp);
    fputws(L"\n", ofp);
  }
  fclose(ifp);
  fclose(ofp);
}
