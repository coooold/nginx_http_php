static sapi_module_struct ngx_api_sapi_module =
{
	"litespeed",
	"LiteSpeed",

	php_ngx_api_startup,              /* startup */
	php_module_shutdown_wrapper,    /* shutdown */

	NULL,                           /* activate */
	NULL,          /* deactivate */

	sapi_ngx_api_ub_write,            /* unbuffered write */
	NULL,               /* flush */
	NULL,                           /* get uid */
	NULL,              /* getenv */

	NULL,                      /* error handler */

	NULL,                           /* header handler */
	NULL,        /* send headers handler */
	NULL,                           /* send header handler */

	NULL,           /* read POST data */
	NULL,        /* read Cookies */

	NULL,  /* register server variables */
	NULL,         /* Log message */

	NULL,                           /* php.ini path override */
	NULL,                           /* block interruptions */
	NULL,                           /* unblock interruptions */
	NULL,                           /* default post reader */
	NULL,                           /* treat data */
	NULL,                           /* executable location */

	0,                              /* php.ini ignore */

	STANDARD_SAPI_MODULE_PROPERTIES

};

static int php_ngx_api_startup(sapi_module_struct *sapi_module)
{
	if (php_module_startup(sapi_module, NULL, 0)==FAILURE) {
		return FAILURE;
	}
	return SUCCESS;
}

static int sapi_ngx_api_ub_write(const char *str, uint str_length TSRMLS_DC)
{
}

