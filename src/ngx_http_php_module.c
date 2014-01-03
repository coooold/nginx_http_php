
/*
 * Copyright (C) Coooold (coooold@gmail.com)
 */

#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include <sapi/embed/php_embed.h>
#include <TSRM.h>
#include <SAPI.h>
#include <zend_ini.h>
#include <php.h>

static void* ngx_http_php_loc_conf(ngx_conf_t* cf);
static ngx_int_t ngx_http_php_handler(ngx_http_request_t* r);
static char* ngx_http_php(ngx_conf_t* cf, ngx_command_t* cmd, void* conf);
//static int php_module_ub_write(const char *str, unsigned int str_length TSRMLS_DC);

static ngx_int_t ngx_http_php_vm_init(ngx_cycle_t *cycle);
static void ngx_http_php_vm_quit(ngx_cycle_t *cycle);

static void ***tsrm_ls;
static ngx_chain_t *out;
static ngx_chain_t *out_current;
static ngx_http_request_t* temp_request;

static zend_module_entry nginx_php_module_entry;
static sapi_module_struct ngx_api_sapi_module;
static int php_ngx_api_startup(sapi_module_struct *sapi_module);
static int sapi_ngx_api_ub_write(const char *str, uint str_length TSRMLS_DC);
static int sapi_php_module_deactivate(TSRMLS_D);
static int sapi_php_module_activate(TSRMLS_D);
static void sapi_php_module_flush( void * server_context );
static char *sapi_php_module_read_cookies(TSRMLS_D);
static int sapi_php_module_read_post(char *buffer, uint count_bytes TSRMLS_DC);
static int sapi_php_module_send_headers(sapi_headers_struct *sapi_headers TSRMLS_DC);

typedef struct{
	ngx_str_t file_name;
	char* file_name_str;
	zend_op_array *op_array;
	int execute_type;
} ngx_http_php_loc_conf_t;


static void runPHP(ngx_http_php_loc_conf_t* plcf);


static ngx_command_t ngx_http_php_commands[] = {
	{
	    ngx_string("php"), // The command name
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_http_php, // The command handler
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_php_loc_conf_t, file_name),
        NULL
	},
	{
	    ngx_string("php_file"), // The command name
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_http_php_file, // The command handler
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_php_loc_conf_t, file_name),
        NULL
	},
	ngx_null_command
};


// 上下文
static ngx_http_module_t ngx_http_php_module_ctx = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    ngx_http_php_loc_conf,
    NULL
};

//初始化loc配置
static void* ngx_http_php_loc_conf(ngx_conf_t* cf) {
    ngx_http_php_loc_conf_t* conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_php_loc_conf_t));
    if (conf == NULL) {
        return NGX_CONF_ERROR;
    }
    conf->file_name.len = 0;
    conf->file_name.data = NULL;
	//conf->op_array = NULL;
    return conf;
}

// 模块定义
ngx_module_t ngx_http_php_module = {
    NGX_MODULE_V1,
    &ngx_http_php_module_ctx,
    ngx_http_php_commands,
    NGX_HTTP_MODULE,
	NULL,	// 初始化 master 时执行
    NULL,	//初始化模块时执行
    ngx_http_php_vm_init,	// 初始化进程时执行
    NULL,	// 初始化线程时执行
    NULL,	// 退出线程时执行
    ngx_http_php_vm_quit,	// 退出进程时执行
    NULL,	// 退出 master 时执行
    NGX_MODULE_V1_PADDING
};


//命令处理器
static char* ngx_http_php(ngx_conf_t* cf, ngx_command_t* cmd, void* conf){
    ngx_http_core_loc_conf_t* clcf;
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_php_handler;
    ngx_conf_set_str_slot(cf, cmd, conf);
		
	ngx_http_php_loc_conf_t* plcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_php_module);
	//write(STDOUT_FILENO, plcf->file_name.data, plcf->file_name.len);
	
	char *file_name = ngx_pcalloc(cf->pool, plcf->file_name.len+1);
    if (conf == NULL) {
        return NGX_CONF_ERROR;
    }

	ngx_memcpy(file_name, plcf->file_name.data, plcf->file_name.len);
	file_name[plcf->file_name.len] = '\0';
	
	plcf->file_name_str = file_name;
	
	//plcf->op_array = compile_php_vm_file(file_name TSRMLS_CC);

    return NGX_CONF_OK;
}

//命令处理器
static char* ngx_http_php_file(ngx_conf_t* cf, ngx_command_t* cmd, void* conf){
    ngx_http_core_loc_conf_t* clcf;
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_php_handler;
    ngx_conf_set_str_slot(cf, cmd, conf);
		
	ngx_http_php_loc_conf_t* plcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_php_module);
	//write(STDOUT_FILENO, plcf->file_name.data, plcf->file_name.len);
	
	char *file_name = ngx_pcalloc(cf->pool, plcf->file_name.len+1);
    if (conf == NULL) {
        return NGX_CONF_ERROR;
    }

	ngx_memcpy(file_name, plcf->file_name.data, plcf->file_name.len);
	file_name[plcf->file_name.len] = '\0';
	
	plcf->file_name_str = file_name;
	
	//plcf->op_array = compile_php_vm_file(file_name TSRMLS_CC);

    return NGX_CONF_OK;
}

static ngx_int_t ngx_http_php_vm_init(ngx_cycle_t *cycle){
	#ifdef ZTS
	tsrm_startup(1, 1, 0, NULL);
	#endif
	
	//printf("ngx_http_php_vm_init");
	sapi_startup(&ngx_api_sapi_module);
	if (ngx_api_sapi_module.startup(&ngx_api_sapi_module) == FAILURE) {
		return FAILURE;
	}
	return 0;
}

static void ngx_http_php_vm_quit(ngx_cycle_t *cycle){
	php_module_shutdown(TSRMLS_C);
	sapi_shutdown();
	tsrm_ls = NULL;
}


//处理器
static ngx_int_t ngx_http_php_handler(ngx_http_request_t* r) {
    ngx_int_t rc;
	//printf("ngx_http_php_handler\n");
    ngx_http_php_loc_conf_t* plcf;
	plcf = ngx_http_get_module_loc_conf(r, ngx_http_php_module);

    //r->headers_out.content_type.len = sizeof("text/plain") - 1;
    //r->headers_out.content_type.data = (u_char*)"text/plain";
	out = NULL;
	out_current = NULL;
	temp_request = r;

	runPHP(plcf);

	if(out == NULL){
		return NGX_ERROR;
	}
/*
	//获取内容长度
	r->headers_out.content_length_n = 0;
	ngx_chain_t *i = out;
	while(i != NULL)
	{
		r->headers_out.content_length_n += (i->buf->last - i->buf->pos);
		i = i->next;
	}
*/
    r->headers_out.status = NGX_HTTP_OK;
    rc = ngx_http_send_header(r);
	
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
        return rc;
    }

    return ngx_http_output_filter(r, out);
}

	
static void runPHP(ngx_http_php_loc_conf_t* plcf){
	//printf("\nrunPHP()\n");

	zend_first_try {
		SG(request_info).content_type = "";
		SG(request_info).request_method = "GET";
		SG(request_info).query_string = "";
		SG(request_info).request_uri = "";
		SG(request_info).content_length = 0;
		SG(request_info).path_translated = "a.php";

		/* It is not reset by zend engine, set it to 0. */
		SG(sapi_headers).http_response_code = 0;
		SG(server_context) = NULL;

		if (php_request_startup(TSRMLS_C) == FAILURE) {
			php_module_shutdown(TSRMLS_C);
			exit(-1);
		}
	
		zend_file_handle script;

		script.type = ZEND_HANDLE_FILENAME;
		script.filename = plcf->file_name_str;
		script.opened_path = NULL;
		script.free_filename = 0;
	
		zend_execute_scripts(ZEND_INCLUDE TSRMLS_CC, NULL, 1, &(script));
		
		php_request_shutdown(NULL);
	} zend_catch {
	} zend_end_try();

}


//////////////////////sapi

static zend_module_entry nginx_php_module_entry = {
	STANDARD_MODULE_HEADER,
	"litespeed",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NO_VERSION_YET,
	STANDARD_MODULE_PROPERTIES
};

static sapi_module_struct ngx_api_sapi_module =
{
	"litespeed",
	"LiteSpeed",

	php_ngx_api_startup,              /* startup */
	php_module_shutdown_wrapper,    /* shutdown */

	sapi_php_module_activate,                           /* activate */
	sapi_php_module_deactivate,          /* deactivate */

	sapi_ngx_api_ub_write,            /* unbuffered write */
	sapi_php_module_flush,				/* flush */
	NULL,							/* get uid */
	NULL,				/* getenv */

	php_error,						/* error handler */

	NULL,							/* header handler */
	sapi_php_module_send_headers,			/* send headers handler */
	NULL,							/* send header handler */

	sapi_php_module_read_post,				/* read POST data */
	sapi_php_module_read_cookies,			/* read Cookies */

	NULL,	/* register server variables */
	NULL,			/* Log message */
	NULL,							/* Get request time */
	NULL,							/* Child terminate */

	NULL,        //char *php_ini_path_override;

    NULL,    //void (*block_interruptions)(void);
    NULL,    //void (*unblock_interruptions)(void);

    NULL,    //void (*default_post_reader)(TSRMLS_D);
    NULL,    //void (*treat_data)(int arg, char *str, zval *destArray TSRMLS_DC);
    NULL,    //char *executable_location;

    0,    //int php_ini_ignore;
    1,    //int php_ini_ignore_cwd; /* don't look for php.ini in the current directory */

    NULL,    //int (*get_fd)(int *fd TSRMLS_DC);

    NULL,    //int (*force_http_10)(TSRMLS_D);

    NULL,    //int (*get_target_uid)(uid_t * TSRMLS_DC);
    NULL,    //int (*get_target_gid)(gid_t * TSRMLS_DC);

    NULL,    //unsigned int (*input_filter)(int arg, char *var, char **val, unsigned int val_len, unsigned int *new_val_len TSRMLS_DC);

    NULL,    //void (*ini_defaults)(HashTable *configuration_hash);
    0,    //int phpinfo_as_text;

    NULL,    //char *ini_entries;
    NULL,    //const zend_function_entry *additional_functions;
    NULL    //unsigned int (*input_filter_init)(TSRMLS_D);
};

static int php_ngx_api_startup(sapi_module_struct *sapi_module)
{
	//printf("--php_ngx_api_startup--");
	if (php_module_startup(sapi_module, &nginx_php_module_entry, 0)) {
        return FAILURE;
    } else {
        return SUCCESS;
    }
}

static int sapi_ngx_api_ub_write(const char *str, uint str_length TSRMLS_DC)
{
	ngx_chain_t *next_out;
	ngx_buf_t* b;
	next_out = (ngx_chain_t*)ngx_pcalloc(temp_request->pool, sizeof(ngx_chain_t));
	b = (ngx_buf_t*)ngx_pcalloc(temp_request->pool, sizeof(ngx_buf_t));
	b->pos = ngx_pcalloc(temp_request->pool, str_length);
	ngx_memcpy(b->pos, str, str_length);
	b->last = b->pos + str_length;
	b->memory = 1;
	b->last_buf = 1;
	next_out->buf = b;
    next_out->next = NULL;
	
	if(out == NULL)
	{
		out = next_out;
		out_current = out;
	}else{
		out_current->next = next_out;
		out_current = next_out;
	}
	
	return SUCCESS;
}

static int sapi_php_module_deactivate(TSRMLS_D)
{
	//printf("\nsapi_php_module_deactivate\n");
	return SUCCESS;
}

static int sapi_php_module_activate(TSRMLS_D)
{
	//printf("\nsapi_php_module_activate\n");
	return SUCCESS;
}

static void sapi_php_module_flush( void * server_context )
{
	//printf("\nsapi_php_module_flush\n");
	//php_handle_aborted_connection();
}

static char *sapi_php_module_read_cookies(TSRMLS_D)
{
	//printf("\nsapi_php_module_read_cookies\n");
	return NULL;
}

static int sapi_php_module_read_post(char *buffer, uint count_bytes TSRMLS_DC)
{
	//printf("\nsapi_php_module_read_post\n");
	return 0;
}

static int sapi_php_module_send_headers(sapi_headers_struct *sapi_headers TSRMLS_DC){
	//printf("\nsapi_php_module_send_headers\n");
	return SAPI_HEADER_SENT_SUCCESSFULLY;
}