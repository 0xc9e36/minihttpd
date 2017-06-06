#ifndef _HTTP_PARSE_HEAD_
#define _HTTP_PARSE_HEAD_

#include "common.h"
#include "http.h"

int http_parse_line(http_request *hr);
int http_parse_head(http_request *hr);
int http_parse_body(http_request *hr);


#endif
