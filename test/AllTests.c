#include <stdio.h>

#include "CuTest.h"

CuSuite* CuGetSuite(void);
CuSuite* CuArrayGetSuite(void);
CuSuite* CuStringGetSuite(void);

int RunAllTests(void) {
    CuString* output = CuStringNew();
    CuSuite* suite = CuSuiteNew();

    CuSuiteAddSuite(suite, CuGetSuite());
    CuSuiteAddSuite(suite, CuArrayGetSuite());
    CuSuiteAddSuite(suite, CuStringGetSuite());

    CuSuiteRun(suite);
    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);
    return suite->failCount;
}

CU_WEAK int main(void) {
    return RunAllTests();
}
