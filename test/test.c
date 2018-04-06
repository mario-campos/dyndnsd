#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "process.c"

void
test_parse_fqdn_2d(void **state) {
	char *fqdn = "google.com";
	char *hostname, *domain, *tld;
	parse_fqdn(fqdn, &hostname, &domain, &tld);
	assert_string_equal(hostname, "@");
	assert_string_equal(domain, "google.com");
	assert_string_equal(tld, "com");
	free(hostname); free(domain); free(tld);
}

void
test_parse_fqdn_3d(void **state) {
	char *fqdn = "www.mariocampos.io";
	char *hostname, *domain, *tld;
	parse_fqdn(fqdn, &hostname, &domain, &tld);
	assert_string_equal(hostname, "www");
	assert_string_equal(domain, "mariocampos.io");
	assert_string_equal(tld, "io");
	free(hostname); free(domain); free(tld);
}

void
test_parse_fqdn_4d(void **state) {
	char *fqdn = "ftp.dyndnsd.mariocampos.io";
	char *hostname, *domain, *tld;
	parse_fqdn(fqdn, &hostname, &domain, &tld);
	assert_string_equal(hostname, "ftp");
	assert_string_equal(domain, "dyndnsd.mariocampos.io");
	assert_string_equal(tld, "io");
	free(hostname); free(domain); free(tld);
}

const struct CMUnitTest cmocka_tests[] = {
	cmocka_unit_test(test_parse_fqdn_2d),
	cmocka_unit_test(test_parse_fqdn_3d),
	cmocka_unit_test(test_parse_fqdn_4d)
};

int
main(void) {
	return cmocka_run_group_tests(cmocka_tests, NULL, NULL);
}
