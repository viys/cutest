# PowerShell 脚本: make-tests.ps1
# 自动生成单一 AllTests 文件的内容，用于 CuTest。
# 在当前目录搜索所有 .c 文件并将结果打印到控制台。

param (
    [string[]]$Files = @() # 可通过命令行参数指定文件，否则默认处理当前目录的 *.c 文件
)

if (-not $Files) {
    $Files = Get-ChildItem -Filter "*.c" | ForEach-Object { $_.FullName }
}

# 输出文件头
@'
/* This is auto-generated code. Edit at your own peril. */
#include <stdio.h>
#include <stdlib.h>
#include "CuTest.h"

'@ 

# 处理函数声明
foreach ($file in $Files) {
    Get-Content $file | Where-Object { $_ -match "^void Test" } |
    ForEach-Object {
        $_ -replace "\(.*$", "(CuTest*);" -replace "^", "extern "
    }
}

# 输出 RunAllTests 函数头
@'

void RunAllTests(void) {
    CuString *output = CuStringNew();
    CuSuite* suite = CuSuiteNew();

'@

# 处理每个测试函数，生成 SUITE_ADD_TEST 调用
foreach ($file in $Files) {
    Get-Content $file | Where-Object { $_ -match "^void Test" } |
    ForEach-Object {
        $_ -replace "^void ", "" -replace "\(.*$", "" -replace "^", "    SUITE_ADD_TEST(suite, " -replace "$", ");"
    }
}

# 输出 RunAllTests 函数尾部
@'

    CuSuiteRun(suite);
    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);
    CuStringDelete(output);
    CuSuiteDelete(suite);
}
'@

# 输出 main 函数
@'

int main(void) {
    RunAllTests();
}
'@
