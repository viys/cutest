#include <stdio.h>
#include <stdlib.h>

#include "CuTest.h"

CuSuite* CuGetSuite(void);
CuSuite* CuArrayGetSuite(void);
CuSuite* CuStringGetSuite(void);

static void AddSuiteAndReleaseShell(CuSuite* suite, CuSuite* source) {
    CuSuiteAddSuite(suite, source);
    free(source);
}

int RunAllTests(void) {
    int failCount;
    CuString* output = CuStringNew();
    CuSuite* suite = CuSuiteNew();

    AddSuiteAndReleaseShell(suite, CuGetSuite());
    AddSuiteAndReleaseShell(suite, CuArrayGetSuite());
    AddSuiteAndReleaseShell(suite, CuStringGetSuite());

    CuSuiteRun(suite);
    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);
    failCount = suite->failCount;
    CuStringDelete(output);
    CuSuiteDelete(suite);
    return failCount;
}

int main(void) {
    return RunAllTests();
}
