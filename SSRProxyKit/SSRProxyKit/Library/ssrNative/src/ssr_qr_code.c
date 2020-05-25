
#include "base64.h"
#include "ssr_executive.h"
#include "ssr_cipher_names.h"
#include "obfs.h" // for SSR_BUFF_SIZE

static const char *ss_header = "ss://";
static const char *ssr_header = "ssr://";
static const char *obfsparam = "obfsparam";
static const char *protoparam = "protoparam";
static const char *remarks = "remarks";
static const char *group = "group";
static const char *udpport = "udpport";
static const char *uot = "uot";

char * ssr_qr_code_encode(const struct server_config *config, void*(*alloc_fn)(size_t size)) {
    if (config==NULL || alloc_fn==NULL) {
        return NULL;
    }

    if (config->remote_host == NULL ||
        config->remote_port == 0 ||
        config->method == NULL ||
        config->password == NULL ||
        config->protocol == NULL ||
        config->obfs == NULL) {
        return NULL;
    }

    // ssr://base64(host:port:protocol:method:obfs:base64pass/?obfsparam=base64param&protoparam=base64param&remarks=base64remarks&group=base64group&udpport=0&uot=0)

    unsigned char *base64_buf = (unsigned char *)calloc(SSR_BUFF_SIZE, sizeof(base64_buf[0]));

    char *basic = (char *)calloc(SSR_BUFF_SIZE, sizeof(basic[0]));

    url_safe_base64_encode((unsigned char *)config->password, (int)strlen(config->password), base64_buf);
    sprintf(basic, "%s:%d:%s:%s:%s:%s",
            config->remote_host, config->remote_port,
            config->protocol, config->method, config->obfs,
            base64_buf);

    char *optional = (char *)calloc(SSR_BUFF_SIZE, sizeof(optional[0]));
    static const char *fmt0 = "%s=%s";
    static const char *fmt1 = "&%s=%s";

    if ((config->obfs_param != NULL) && (strlen(config->obfs_param) != 0)) {
        size_t len = strlen(optional);
        memset(base64_buf, 0, SSR_BUFF_SIZE*sizeof(base64_buf[0]));
        url_safe_base64_encode((unsigned char *)config->obfs_param, (int)strlen(config->obfs_param), base64_buf);
        sprintf(optional+len, len?fmt1:fmt0, obfsparam, base64_buf);
    }
    if ((config->protocol_param != NULL) && (strlen(config->protocol_param) != 0)) {
        size_t len = strlen(optional);
        memset(base64_buf, 0, SSR_BUFF_SIZE*sizeof(base64_buf[0]));
        url_safe_base64_encode((unsigned char *)config->protocol_param, (int)strlen(config->protocol_param), base64_buf);
        sprintf(optional+len, len?fmt1:fmt0, protoparam, base64_buf);
    }
    if ((config->remarks != NULL) && (strlen(config->remarks) != 0)) {
        size_t len = strlen(optional);
        memset(base64_buf, 0, SSR_BUFF_SIZE*sizeof(base64_buf[0]));
        url_safe_base64_encode((unsigned char *)config->remarks, (int)strlen(config->remarks), base64_buf);
        sprintf(optional+len, len?fmt1:fmt0, remarks, base64_buf);
    }
    // config->group
    // config->udpport
    // config->uot

    char *result = (char *)alloc_fn(SSR_BUFF_SIZE * sizeof(result[0]));
    sprintf(result, strlen(optional) ? "%s/?%s" : "%s/%s", basic, optional);
    
    memset(base64_buf, 0, SSR_BUFF_SIZE*sizeof(base64_buf[0]));
    url_safe_base64_encode((unsigned char *)result, (int)strlen(result), base64_buf);

    sprintf(result, "%s%s", ssr_header, base64_buf);
    
    free(base64_buf);
    free(basic);
    free(optional);

    return result;
}

struct server_config * decode_shadowsocks(const char *text);
struct server_config * decode_ssr(const char *text);

struct server_config * ssr_qr_code_decode(const char *text) {
    if (text == NULL || strlen(text)==0) {
        return NULL;
    }
    size_t hdr_len = strlen(ss_header);
    if (strncmp(text, ss_header, hdr_len) == 0) {
        return decode_shadowsocks(text);
    }
    
    hdr_len = strlen(ssr_header);
    if (strncmp(text, ssr_header, hdr_len) == 0) {
        return decode_ssr(text);
    }
    
    return NULL;
}

//
// ss:// BASE64-ENCODED-STRING-WITHOUT-PADDING(method:password@hostname:port) # remarks
//
struct server_config * decode_shadowsocks(const char *text) {
    struct server_config *config = NULL;
    char *contents = NULL;
    char *plain_text = NULL;

    do {
        if (text == NULL || strlen(text)==0) {
            break;
        }
        size_t hdr_len = strlen(ss_header);
        if (strncmp(text, ss_header, hdr_len) != 0) {
            break;
        }
        text = text + hdr_len;
        
        contents = strdup(text);

        char *remarks = strchr(contents, '#');
        
        if (remarks != NULL) {
            *remarks++ = '\0';
        }

        if (strcspn(contents, "@:/?") != strlen(contents)) {
            // SS AEAD not support forever.
            break;
        }

        int len = url_safe_base64_decode_len((unsigned char *)contents);
        plain_text = (char *) calloc(len+1, sizeof(plain_text[0]));
        url_safe_base64_decode((const unsigned char *)contents, (unsigned char *)plain_text);
        
        char *method = plain_text;

        char *password = strchr(plain_text, ':');
        if (password == NULL) {
            break;
        }
        *password++ = '\0';

        char *port = strrchr(password, ':');
        if (port == NULL) {
            break;
        }
        *port++ = '\0';
        
        char *hostname = strrchr(password, '@');
        if (hostname == NULL) {
            break;
        }
        *hostname++ = '\0';

        config = config_create();
        string_safe_assign(&config->method, method);
        string_safe_assign(&config->password, password);
        string_safe_assign(&config->remote_host, hostname);
        config->remote_port = atoi(port);
        if (remarks) {
            string_safe_assign(&config->remarks, remarks);
        }

        const char *t;

        t = ssr_protocol_name_of_type(ssr_protocol_origin);
        string_safe_assign(&config->protocol, t);
        
        t = ssr_obfs_name_of_type(ssr_obfs_plain);
        string_safe_assign(&config->obfs, t);
    } while (0);
    
    if (plain_text) {
        free(plain_text);
    }
    
    if (contents) {
        free(contents);
    }
    
    return config;
}

struct server_config * decode_ssr(const char *text) {
    struct server_config *config = NULL;
    unsigned char *swap_buf = NULL;
    char *plain_text = NULL;
    
    do {
        if (text == NULL || strlen(text)==0) {
            break;
        }
        size_t hdr_len = strlen(ssr_header);
        if (strncmp(text, ssr_header, hdr_len) != 0) {
            break;
        }
        text = text + hdr_len;
        
        int len = url_safe_base64_decode_len((unsigned char *)text);
        plain_text = (char *) calloc(len+1, sizeof(plain_text[0]));
        url_safe_base64_decode((const unsigned char *)text, (unsigned char *)plain_text);
        
        char *basic = plain_text;
        
        char *optional = strchr(plain_text, '/');
        if (optional != NULL) {
            *optional++ = '\0';
            if (*optional == '?') {
                *optional++ = '\0';
            }
        }
        
        char *base64pass = strrchr(basic, ':');
        if (base64pass == NULL) {
            break;
        }
        *base64pass++ = '\0';

        char *obfs = strrchr(basic, ':');
        if (obfs == NULL) {
            break;
        }
        *obfs++ = '\0';
        
        char *method = strrchr(basic, ':');
        if (method == NULL) {
            break;
        }
        *method++ = '\0';
        
        char *protocol = strrchr(basic, ':');
        if (protocol == NULL) {
            break;
        }
        *protocol++ = '\0';
        
        char *port = strrchr(basic, ':');
        if (port == NULL) {
            break;
        }
        *port++ = '\0';
        
        char *host = basic;
        
        swap_buf = (unsigned char *) calloc(SSR_BUFF_SIZE, sizeof(swap_buf[0]));
        
        config = config_create();
        
        string_safe_assign(&config->remote_host, host);
        config->remote_port = atoi(port);
        string_safe_assign(&config->protocol, protocol);
        string_safe_assign(&config->method, method);
        string_safe_assign(&config->obfs, obfs);
        url_safe_base64_decode((unsigned char *)base64pass, swap_buf);
        string_safe_assign(&config->password, (char *)swap_buf);

        if (optional==NULL || strlen(optional)==0) {
            break;
        }
        
        const char *params[] = {
            obfsparam,
            protoparam,
            remarks,
            group,
            udpport,
            uot,
        };
        
        char *iter = optional;
        
        do {
            char *next = strchr(iter, '&');
            if (next) {
                *next++ = 0;
            }
            
            char *key = iter;
            char *value = strchr(iter, '=');
            if (value) {
                *value++ = 0;
                
                for (int i=0; i<sizeof(params)/sizeof(params[0]); ++i) {
                    if (strcmp(params[i], key) != 0) {
                        continue;
                    }
                    switch (i) {
                        case 0:
                            url_safe_base64_decode((unsigned char *)value, swap_buf);
                            string_safe_assign(&config->obfs_param, (char *)swap_buf);
                            break;
                        case 1:
                            url_safe_base64_decode((unsigned char *)value, swap_buf);
                            string_safe_assign(&config->protocol_param, (char *)swap_buf);
                            break;
                        case 2:
                            url_safe_base64_decode((unsigned char *)value, swap_buf);
                            string_safe_assign(&config->remarks, (char *)swap_buf);
                            break;
                        case 3:
                            // config->group
                            break;
                        case 4:
                            // config->udpport
                            break;
                        case 5:
                            // config->uot
                            break;
                        default:
                            break;
                    }
                    break;
                }
            }
            iter = next;
        } while (iter);
    } while (0);
    
    if (plain_text) {
        free(plain_text);
    }
    
    if (swap_buf) {
        free(swap_buf);
    }
    
    return config;
}
