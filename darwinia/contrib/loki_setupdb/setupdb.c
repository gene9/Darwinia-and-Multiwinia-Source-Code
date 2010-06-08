/* Implementation of the Loki Product DB API */
/* $Id: setupdb.c,v 1.71 2005/02/09 23:53:26 megastep Exp $ */

#include "config.h"
#include <glob.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <ctype.h>
#include <parser.h>
#include <tree.h>
#include <xmlmemory.h>

#ifdef HAVE_SYS_MKDEV_H
#include <sys/mkdev.h>
#endif
#ifdef HAVE_SYS_SYSMACROS_H
#include <sys/sysmacros.h>
#endif

#include "setupdb.h"
#include "arch.h"
#include "md5.h"

typedef struct _loki_envvar_t 
{
	xmlNodePtr node;
	char *name;
	char *value;
	struct _loki_envvar_t *next;
} product_envvar_t;

struct _loki_product_t
{
    xmlDocPtr doc;
    product_info_t info;
    int changed;
    product_component_t *components, *default_comp;
	/* Environment variables */
	product_envvar_t *envvars;
};

struct _loki_product_component_t
{
    xmlNodePtr node;
    product_t *product;
    char *name;
    char *version;
    char *url;
    int is_default;
    product_option_t *options;
    product_file_t *scripts;
	/* Environment variables */
	product_envvar_t *envvars;
    product_component_t *next;
};

struct _loki_product_option_t
{
    xmlNodePtr node;
    product_component_t *component;
    char *name;
	char *tag;
    product_option_t *next;    
    product_file_t *files;
};

struct _loki_product_file_t {
    xmlNodePtr node;
    product_option_t *option;
    char *path;
    file_type_t type;
    unsigned int mode;
    unsigned int patched : 1;
	unsigned int mutable : 1;
	char *desktop;
    union {        
        unsigned char md5sum[16];
        script_type_t scr_type;
    } data;
    product_file_t *next;
};


/* Enumerate all products, returns name or NULL if end of list */

static glob_t globbed;
static int glob_index;

static const char *script_types[] = { "pre-uninstall", "post-uninstall" };

static const char *get_productname(char *inipath)
{
    char *ret = strrchr(inipath, '/') + 1, *ptr;
    
    ptr = strstr(ret, ".xml");
    *ptr = '\0';
    return ret;
}

static char *expand_path(product_t *prod, const char *path, char *buf, size_t len)
{
    if ( *path == '/' ) {
        strncpy(buf, path, len);
    } else {
        snprintf(buf, len, "%s/%s", prod->info.root, path);
    }
    return loki_trim_slashes(buf);
}

const char *loki_basename(const char *file)
{
	if ( file ) {
		const char *ret = strrchr(file, '/');
		if ( ret ) {
			return ret+1;
		}
	}
	return file;
}

/* Remove the install path component from the filename to obtain a relative path */
const char *loki_remove_root(const product_t *prod, const char *path)
{
	if ( prod ) {
		if ( !strcmp(path, prod->info.root) )
			return "";
		if ( strstr(path, prod->info.root) == path) {
			path += strlen(prod->info.root) + 1;
			/* Remove any extraneous slashes */
			while ( *path && *path == '/' )
				path ++;
		}
	}
	/* Remove ./ if it is following */
	if ( path[0]=='.' && path[1]=='/') {
		path += 2;
	}
	return path;
}

struct subst {
    char ch;
    const char *str;
};

#define NUM_XML_SUBSTS 5

/* Substitute special characters with XML tokens */
static const char *substitute_xml_string(const char *str)
{
    const char *ptr;
    char *ret;
    int i, count = 0;
    static char buf[PATH_MAX*2];
    static struct subst substs[NUM_XML_SUBSTS] = {
        { '&', "&amp;" },
        { '<', "&lt;" },
        { '>', "&gt;" },
        { '\'', "&apos;" },
        { '"', "&quot;" }
    };

    ret = buf;
    while ( *str ) {
        for ( i = 0; i < NUM_XML_SUBSTS; ++i ) {
            if ( substs[i].ch == *str ) {
                for ( ptr = substs[i].str; *ptr && count<sizeof(buf); ptr ++) {
                    *ret ++ = *ptr;
                    count ++;
                }
                break;
            }
        }
        if ( i == NUM_XML_SUBSTS && count<sizeof(buf) ) {
            *ret ++ = *str;
            count ++;
        }
        str ++;
    }
    if ( count<sizeof(buf) )
        *ret ++ = '\0';
    return buf;
}

static const char *get_xml_string(product_t *product, xmlNodePtr node)
{
    const char *text = xmlNodeListGetString(product->doc, node->childs, 1);
    return text;
}

static void insert_end_file(product_file_t *file, product_file_t **opt)
{
    file->next = NULL;
    if ( *opt ) {
        product_file_t *it;
        for( it = *opt; it; it = it->next ) {
            if ( it->next == NULL ) {
                it->next = file;
                return;
            }
        }
    } else {
        *opt = file;
    }
}

static const char *get_xml_base(void)
{
    const char *base;

    base = getenv("SETUPDB_XML_BASE");
    if ( ! base ) {
        base = ".";
    }
    return(base);
}

const char *loki_getfirstproduct(void)
{
    char buf[PATH_MAX];

    snprintf(buf, sizeof(buf), "%s/" LOKI_DIRNAME "/installed/%s/*.xml", detect_home(), get_xml_base());
    if ( glob(buf, GLOB_ERR, NULL, &globbed) != 0 ) {
        return NULL;
    } else {
        if ( globbed.gl_pathc == 0 ) {
            return NULL;
        } else {
            glob_index = 1;
            return get_productname(globbed.gl_pathv[0]);
        }
    }
}

const char *loki_getnextproduct(void)
{
    if ( glob_index >= globbed.gl_pathc ) {
        return NULL;
    } else {
        return get_productname(globbed.gl_pathv[glob_index++]);
    }
}

/* Extract base and extension from a version string */
void loki_split_version(const char *version, char *base, int maxbase,
                                             char *ext, int maxext)
{
    if ( *version == '*' ) {
        *base++ = *version++;
    } else {
        while ( --maxbase && (isalnum((int)*version) || (*version == '.')) ) {
            *base++ = *version++;
        }
    }
    *base = '\0';

    while ( --maxext && *version ) {
        *ext++ = *version++;
    }
    *ext = '\0';
}

static void get_word(const char **string, char *word, int maxlen)
{
    while ( (maxlen > 0) && **string && isalpha((int)**string) ) {
        *word = **string;
        ++word;
        ++*string;
        --maxlen;
    }
    *word = '\0';
}

static int get_num(const char **string)
{
    int num;

    num = atoi(*string);
    while ( isdigit((int)**string) ) {
        ++*string;
    }
    return(num);
}

int loki_newer_version(const char *version1, const char *version2)
{
    int newer;
    int num1, num2;
    char base1[128], ext1[128];
    char base2[128], ext2[128];
    char word1[32], word2[32];

    /* Compare sequences of numbers and letters in the base versions */
    newer = 0;
    loki_split_version(version1, base1, sizeof(base1), ext1, sizeof(ext1));
    version1 = base1;
    loki_split_version(version2, base2, sizeof(base2), ext2, sizeof(ext2));
    version2 = base2;
    while ( !newer && (*version1 && *version2) &&
            ((isdigit((int)*version1) && isdigit((int)*version2)) ||
             (isalpha((int)*version1) && isalpha((int)*version2))) ) {
        if ( isdigit((int)*version1) ) {
            num1 = get_num(&version1);
            num2 = get_num(&version2);
            if ( num1 != num2 ) {
                return(num1 > num2);
            }
        } else {
            get_word(&version1, word1, sizeof(word1));
            get_word(&version2, word2, sizeof(word2));
            if ( strcasecmp(word1, word2) != 0 ) {
                return (strcasecmp(word1, word2) > 0);
            }
        }
        if ( *version1 == '.' ) {
            ++version1;
        }
        if ( *version2 == '.' ) {
            ++version2;
        }
    }
    if ( isalpha((int)*version1) && !isalpha((int)*version2) ) {
        newer = 1;
    }
    return(newer);
}


/* Open a product by name*/

product_t *loki_openproduct(const char *name)
{
    char buf[PATH_MAX];
    int major, minor;
    char *str;
    xmlDocPtr doc = NULL;
    xmlNodePtr node;
    product_t *prod;

    if ( strchr(name, '/') != NULL ) { /* Absolute path to a manifest file */
        doc = xmlParseFile(name);
    } else {
        /* Look for a matching case-insensitive file */
        static glob_t xmls;
        
        snprintf(buf, sizeof(buf), "%s/" LOKI_DIRNAME "/installed/%s/*.xml", detect_home(), get_xml_base());
        if ( glob(buf, GLOB_ERR, NULL, &xmls) != 0 ) {
            return NULL;
        } else {
            if ( xmls.gl_pathc == 0 ) {
                return NULL;
            } else {
                int i;
                for ( i = 0; i < xmls.gl_pathc; ++i ) {
                    if ( !strcasecmp(name, get_productname(xmls.gl_pathv[i])) ) {
                        /* The .xml extension was removed by get_productname() */
                        snprintf(buf, sizeof(buf), "%s.xml", xmls.gl_pathv[i]);
                        doc = xmlParseFile(buf);                                
                        break;
                    }
                }
            }
        }
    }
    if ( !doc )
        return NULL;
    prod = (product_t *)malloc(sizeof(product_t));
    prod->doc = doc;
    prod->changed = 0;

    str = xmlGetProp(doc->root, "name");
    strncpy(prod->info.name, str, sizeof(prod->info.name));
	xmlFree(str);
    str = xmlGetProp(doc->root, "desc");
    if ( str ) {
        strncpy(prod->info.description, str, sizeof(prod->info.description));
		xmlFree(str);
    } else {
        *prod->info.description = '\0';
    }
    str = xmlGetProp(doc->root, "root");
    strncpy(prod->info.root, str, sizeof(prod->info.root));
	xmlFree(str);
    str = xmlGetProp(doc->root, "prefix");
	if ( str ) {
		strncpy(prod->info.prefix, str, sizeof(prod->info.prefix));
		xmlFree(str);
	} else {
		strcpy(prod->info.prefix, ".");
	}
    str = xmlGetProp(doc->root, "update_url");
    strncpy(prod->info.url, str, sizeof(prod->info.url));
	xmlFree(str);
    if ( *name == '/' ) { /* Absolute path to a manifest.ini file */
        strncpy(prod->info.registry_path, name,
                sizeof(prod->info.registry_path));
    } else {
        snprintf(prod->info.registry_path, sizeof(prod->info.registry_path),
                 "%s/.manifest/%s.xml", prod->info.root, prod->info.name);
    }

    /* Check for the xmlversion attribute for backwards compatibility */
    str = xmlGetProp(doc->root, "xmlversion");
    sscanf(str, "%d.%d", &major, &minor);
    if ( (major > SETUPDB_VERSION_MAJOR) || 
         ((major == SETUPDB_VERSION_MAJOR) && (minor > SETUPDB_VERSION_MINOR)) ) {
        fprintf(stderr, "Warning: This XML file was generated with a later version of setupdb (%d.%d).\n"
                "Problems may occur.\n", major, minor);
    }
	xmlFree(str);

    prod->components = prod->default_comp = NULL;
	prod->envvars = NULL;

    /* Parse the XML tags and build a tree. Water every day so that it grows steadily. */
    
    for ( node = doc->root->childs; node; node = node->next ) {
        if ( !strcmp(node->name, "component") ) {
            xmlNodePtr optnode;

            product_component_t *comp = (product_component_t *) malloc(sizeof(product_component_t));

            comp->node = node;
            comp->product = prod;
            comp->name = xmlGetProp(node, "name");
            comp->version = xmlGetProp(node, "version");
            comp->url = xmlGetProp(node, "update_url");
            str = xmlGetProp(node, "default");
            comp->is_default = (str && *str=='y');
			xmlFree(str);
            if ( comp->is_default ) {
                prod->default_comp = comp;
            }
            comp->options = NULL;
            comp->scripts = NULL;
			comp->envvars = NULL;
            comp->next = prod->components;
            prod->components = comp;

            for ( optnode = node->childs; optnode; optnode = optnode->next ) {
                if ( !strcmp(optnode->name, "option") ) {
                    xmlNodePtr filenode;
                    product_option_t *opt = (product_option_t *)malloc(sizeof(product_option_t));

                    opt->node = optnode;
                    opt->component = comp;
                    opt->name = xmlGetProp(optnode, "name");
					opt->tag = xmlGetProp(optnode, "tag");
                    opt->files = NULL;
                    opt->next = comp->options;
                    comp->options = opt;

                    for( filenode = optnode->childs; filenode; filenode = filenode->next ) {
                        file_type_t t;
                        product_file_t *file = (product_file_t *) malloc(sizeof(product_file_t));
						const char *cstr;

                        memset(file->data.md5sum, 0, 16);
                        if ( !strcmp(filenode->name, "file") ) {
                            char *md5 = xmlGetProp(filenode, "md5");
                            t = LOKI_FILE_REGULAR;
                            if ( md5 ) {
                                memcpy(file->data.md5sum, get_md5_bin(md5), 16);
								xmlFree(md5);
							}
                        } else if ( !strcmp(filenode->name, "directory") ) {
                            t = LOKI_FILE_DIRECTORY;
                        } else if ( !strcmp(filenode->name, "symlink") ) {
                            t = LOKI_FILE_SYMLINK;
                        } else if ( !strcmp(filenode->name, "fifo") ) {
                            t = LOKI_FILE_FIFO;
                        } else if ( !strcmp(filenode->name, "device") ) {
                            t = LOKI_FILE_DEVICE;
                        } else if ( !strcmp(filenode->name, "rpm") ) {
                            t = LOKI_FILE_RPM;
                        } else if ( !strcmp(filenode->name, "script") ) {
                            t = LOKI_FILE_SCRIPT;
                            str = xmlGetProp(filenode, "type");
                            if ( str ) {
                                if ( !strcmp(str, script_types[LOKI_SCRIPT_PREUNINSTALL]) ) {
                                    file->data.scr_type = LOKI_SCRIPT_PREUNINSTALL;
                                } else if ( !strcmp(str, script_types[LOKI_SCRIPT_POSTUNINSTALL]) ) {
                                    file->data.scr_type = LOKI_SCRIPT_POSTUNINSTALL;
                                }
								xmlFree(str);
                            }
                        } else {
                            t = LOKI_FILE_NONE;
                        }
                        file->node = filenode;
                        file->option = opt;
                        file->type = t;
                        str = xmlGetProp(filenode, "patched");
                        if ( str && *str=='y' ) {
                            file->patched = 1;
                        } else {
                            file->patched = 0;
                        }
						xmlFree(str);
                        str = xmlGetProp(filenode, "mutable");
                        if ( str && *str=='y' ) {
                            file->mutable = 1;
                        } else {
                            file->mutable = 0;
                        }
						xmlFree(str);
                        str = xmlGetProp(filenode, "desktop");
                        if ( str ) {
							file->desktop = strdup(str);
							xmlFree(str);
                        } else {
							file->desktop = NULL;
						}
                        str = xmlGetProp(filenode, "mode");
                        if ( str ) {
                            sscanf(str,"%o", &file->mode);
                        } else {
                            file->mode = 0644;
                        }
						xmlFree(str);
                        cstr = get_xml_string(prod, filenode);
                        file->path = strdup(cstr); /* The expansion is done in loki_getname_file() */

                        insert_end_file(file, &opt->files);
                    }
                } else if ( !strcmp(optnode->name, "script") ) {
                    product_file_t *file = (product_file_t *) malloc(sizeof(product_file_t));
                    file->node = optnode;
                    file->type = LOKI_FILE_SCRIPT;
					file->desktop = NULL;

                    str = xmlGetProp(optnode, "type");
                    if ( str ) {
                        if ( !strcmp(str, script_types[LOKI_SCRIPT_PREUNINSTALL]) ) {
                            file->data.scr_type = LOKI_SCRIPT_PREUNINSTALL;
                        } else if ( !strcmp(str, script_types[LOKI_SCRIPT_POSTUNINSTALL]) ) {
                            file->data.scr_type = LOKI_SCRIPT_POSTUNINSTALL;
                        }
						xmlFree(str);
                    }
                    file->path = strdup(get_xml_string(prod, optnode));
                    file->next = comp->scripts;                    
                    comp->scripts = file;
                } else if ( !strcmp(optnode->name, "environment") ) {
					product_envvar_t *var = malloc(sizeof(product_envvar_t));
					
					var->node = node;
					var->name = xmlGetProp(node, "var");
					var->value = xmlGetProp(node, "value");
					var->next = comp->envvars;
					comp->envvars = var;
				}
            }
        } else if ( !strcmp(node->name, "environment") ) {
			product_envvar_t *var = malloc(sizeof(product_envvar_t));

			var->node = node;
			var->name = xmlGetProp(node, "var");
			var->value = xmlGetProp(node, "value");
			var->next = prod->envvars;
			prod->envvars = var;
		}
    }

    return prod;
}

char *loki_trim_slashes(char *str)
{
	char *ptr = str+strlen(str);
	while ( ptr > str ) {
		ptr--;
		if ( *ptr != '/' ) {
			*(ptr+1) = '\0';
			break;
		}
	}
	return str;
}

/* Create a new product entry */

product_t *loki_create_product(const char *name, const char *root, const char *desc, const char *url)
{
    char homefile[PATH_MAX], manifest[PATH_MAX], myroot[PATH_MAX];
    xmlDocPtr doc;
    product_t *prod;

	/* Create hierarchy if it doesn't exist already */
	snprintf(homefile, sizeof(homefile), "%s/" LOKI_DIRNAME, detect_home());
	mkdir(homefile, 0700);

	strncat(homefile, "/installed", sizeof(homefile)-strlen(homefile));
	mkdir(homefile, 0700);

	strncat(homefile, "/", sizeof(homefile)-strlen(homefile));
	strncat(homefile, get_xml_base(), sizeof(homefile)-strlen(homefile));
	mkdir(homefile, 0700);

	/* Clean up the root - it can't have a trailing slash */
	strncpy(myroot, root, sizeof(myroot));
	loki_trim_slashes(myroot);

    snprintf(manifest, sizeof(manifest), "%s/.manifest", myroot);
    mkdir(manifest, 0755);

    /* Create a new XML tree from scratch; we'll have to dump it to disk later */
    doc = xmlNewDoc("1.0");

    if ( !doc ) {
        return NULL;
    }

	strncat(homefile, "/", sizeof(homefile)-strlen(homefile));
	strncat(homefile, name, sizeof(homefile)-strlen(homefile));
	strncat(homefile, ".xml", sizeof(homefile)-strlen(homefile));

	strncat(manifest, "/", sizeof(manifest)-strlen(manifest));
	strncat(manifest, name, sizeof(manifest)-strlen(manifest));
	strncat(manifest, ".xml", sizeof(manifest)-strlen(manifest));


    /* Symlink the file in the 'installed' per-user directory */
    unlink(homefile);
    if ( symlink(manifest, homefile) < 0 )
        fprintf(stderr, "Could not create symlink : %s\n", homefile);

	/* Create the scripts subdirectory */
	snprintf(manifest, sizeof(manifest), "%s/.manifest/scripts", root);
	mkdir(manifest, 0755);

    prod = (product_t *)malloc(sizeof(product_t));
    prod->doc = doc;
    prod->changed = 1;
    prod->components = prod->default_comp = NULL;
	prod->envvars = NULL;

    doc->root = xmlNewDocNode(doc, NULL, "product", NULL);

    strncpy(prod->info.name, name, sizeof(prod->info.name));
    xmlSetProp(doc->root, "name", name);
    if ( desc ) {
        strncpy(prod->info.description, desc, sizeof(prod->info.description));
        xmlSetProp(doc->root, "desc", desc);
    } else {
        *prod->info.description = '\0';
    }
    snprintf(homefile, sizeof(homefile), "%d.%d", SETUPDB_VERSION_MAJOR, SETUPDB_VERSION_MINOR);
    xmlSetProp(doc->root, "xmlversion", homefile);
    strncpy(prod->info.root, myroot, sizeof(prod->info.root));
    xmlSetProp(doc->root, "root", myroot);
    strncpy(prod->info.url, url, sizeof(prod->info.url));
    strcpy(prod->info.prefix, ".");
    xmlSetProp(doc->root, "update_url", url);
    snprintf(prod->info.registry_path, sizeof(prod->info.registry_path), "%s/.manifest/%s.xml",
             myroot, name);

    return prod;
}

/* Set the install path of a product */

void loki_setroot_product(product_t *product, const char *root)
{
    strncpy(product->info.root, root, sizeof(product->info.root));
    xmlSetProp(product->doc->root, "root", root);
    product->changed = 1;
}

/* Set a path prefix for the installation media for the product */
void loki_setprefix_product(product_t *product, const char *prefix)
{
    strncpy(product->info.prefix, prefix, sizeof(product->info.prefix));
    xmlSetProp(product->doc->root, "prefix", prefix);
    product->changed = 1;
}

/* Set the update URL of a product */

void loki_setupdateurl_product(product_t *product, const char *url)
{
    strncpy(product->info.url, url, sizeof(product->info.url));
    xmlSetProp(product->doc->root, "update_url", url);
    product->changed = 1;
}

/* Close a product entry and free all allocated memory.
   Also writes back to the database all changes that may have been made.
 */

int loki_closeproduct(product_t *product)
{
    int ret = 0;
    product_component_t *comp, *next;
	product_envvar_t *var, *nextvar;

    if ( product->changed ) {
        char tmp[PATH_MAX];
        /* This isn't harmful as long as it's not a world writeable directory */
        snprintf(tmp, sizeof(tmp), "%s.%05d", product->info.registry_path, getpid());
        /* Write XML file to disk if it has changed */
        xmlSaveFile(tmp, product->doc);

        if(rename(tmp, product->info.registry_path) != 0)
        {
            /* too bad but we can't do much about it */
            fprintf(stderr, "Unable to overwrite %s: %s.\nRegistry saved as %s.\n",
                    product->info.registry_path, strerror(errno), tmp);
            ret = -1;
        }
    }

    /* Go through all the allocated structs */
    comp = product->components;
    while ( comp ) {
        product_option_t *opt, *nextopt;
        product_file_t   *scr, *nextscr;

        next = comp->next;
        
        opt = comp->options;
        while ( opt ) {
            product_file_t *file, *nextfile;
            nextopt = opt->next;

            file = opt->files;
            while ( file ) {
                nextfile = file->next;
                free(file->path);
                free(file);
                file = nextfile;
            }

            free(opt->name);
			if ( opt->tag )
				free(opt->tag);
            free(opt);
            opt = nextopt;
        }

        scr = comp->scripts;
        while ( scr ){
            nextscr = scr->next;

            free(scr->path);
            free(scr);
            scr = nextscr;
        }

		var = comp->envvars;
		while ( var ) {
			nextvar = var->next;
			free(var->name);
			free(var->value);
			free(var);
			var = nextvar;
		}
        
        free(comp->name);
        free(comp->version);
        free(comp->url);
        free(comp);
        comp = next;
    }

	var = product->envvars;
	while ( var ) {
		nextvar = var->next;
		free(var->name);
		free(var->value);
		free(var);
		var = nextvar;
	}

    free(product);
    return ret;
}

/* Clean up a product from the registry, i.e. removes all support files and directories */
int loki_removeproduct(product_t *product)
{
    char buf[PATH_MAX];
    product_file_t *file;
    product_option_t *opt;
    product_component_t *comp;

    /* Remove the remaining scripts for each component and options */
    for ( comp = product->components; comp; comp = comp->next ) {
        for ( file = comp->scripts; file; file = file->next ) {
            snprintf(buf, sizeof(buf), "%s/.manifest/scripts/%s.sh",
                     product->info.root, file->path);
            unlink(buf);
        }
        
        for ( opt = comp->options; opt; opt = opt->next ) {
            for ( file = opt->files; file; file = file->next ) {
                if( file->type == LOKI_FILE_SCRIPT ) {
                    snprintf(buf, sizeof(buf), "%s/.manifest/scripts/%s.sh",
                             product->info.root, file->path);
                    unlink(buf);
                }
            }
        }
    }

    /* Remove the XML file */
    unlink(product->info.registry_path);

    /* Remove the directories */
    snprintf(buf, sizeof(buf), "%s/.manifest/scripts", product->info.root);
    if(rmdir(buf)<0)
        perror("Could not remove scripts directory");

    snprintf(buf, sizeof(buf), "%s/.manifest", product->info.root);
    if(rmdir(buf)<0)
        perror("Could not remove .manifest directory");

    /* We can't be removing the current directory, move somewhere else! */
    chdir("/");

    if(rmdir(product->info.root) < 0)
        perror("Could not remove install directory");

    /* Remove the symlink */
    snprintf(buf, sizeof(buf), "%s/" LOKI_DIRNAME "/installed/%s/%s.xml", detect_home(), get_xml_base(), product->info.name);
    unlink(buf);

	/* Change the flag so we won't try to save the file */
    product->changed = 0;
    loki_closeproduct(product);

    return 0;
}

/* Get a pointer to the product info */

product_info_t *loki_getinfo_product(product_t *product)
{
    return &product->info;
}

/* Enumerate the installed options */

product_component_t *loki_getfirst_component(product_t *product)
{
    return product->components;
}

product_component_t *loki_getnext_component(product_component_t *component)
{
    return component ? component->next : NULL;
}

product_component_t *loki_getdefault_component(product_t *product)
{
    return product->default_comp;
}

product_component_t *loki_find_component(product_t *product, const char *name)
{
    product_component_t *ret = product->components;
    while ( ret ) {
        if ( !strcmp(ret->name, name) ) {
            return ret;
        }
        ret = ret->next;
    }
    return NULL;
}

const char *loki_getname_component(product_component_t *component)
{
    return component->name;
}

const char *loki_getversion_component(product_component_t *component)
{
    return component->version;
}

/* Uninstallation messages displayed to the user when the component is removed */
const char *loki_getmessage_component(product_component_t *comp)
{
	xmlNodePtr node;

	/* Look for a <message> tag */
	for ( node = comp->node->childs; node; node = node->next ) {
		if ( node->name && !strcmp(node->name, "message") ) {
			return get_xml_string(comp->product, node);
		}
	}
	return NULL;
}

void loki_setmessage_component(product_component_t *comp, const char *msg)
{
	xmlNodePtr node;

	comp->product->changed = 1;
	/* Look for a <message> tag */
	for ( node = comp->node->childs; node; node = node->next ) {
		if ( node->name && !strcmp(node->name, "message") ) {
			/* Remove the existing node */
			xmlUnlinkNode(node);
			xmlFreeNode(node);
			break;
		}
	}

	if ( msg ) {
		xmlNewChild(comp->node, NULL, "message", substitute_xml_string(msg));
	}
}

size_t loki_getsize_component(product_component_t *component)
{
    size_t size = 0;
    product_option_t *option;

    for ( option = loki_getfirst_option(component);
          option;
          option = loki_getnext_option(option) ) {
        size += loki_getsize_option(option);
    }
    return size;
}

int loki_isdefault_component(product_component_t *comp)
{
    return comp->is_default;
}

void loki_setdefault_component(product_component_t *comp)
{
    product_t *prod = comp->product;
    if ( prod->default_comp ) {
        xmlSetProp(prod->default_comp->node, "default", NULL);
        prod->default_comp->is_default = 0;
    }
    xmlSetProp(comp->node, "default", "yes");
    prod->default_comp = comp;
    prod->changed = 1;
}

product_component_t *loki_create_component(product_t *product, const char *name, const char *version)
{
    xmlNodePtr node = xmlNewChild(product->doc->root, NULL, "component", NULL);
    if ( node ) {
        product_component_t *ret = (product_component_t *)malloc(sizeof(product_component_t));
        ret->node = node;
        ret->product = product;
        ret->name = strdup(name);
        ret->version = strdup(version);
        ret->url = NULL;
        ret->is_default = (product->default_comp == NULL);
        product->changed = 1;
        xmlSetProp(node, "name", name);
        xmlSetProp(node, "version", version);
        if(ret->is_default) {
            xmlSetProp(node, "default", "yes");
            product->default_comp = ret;
        }
        ret->scripts = NULL;
        ret->options = NULL;
		ret->envvars = NULL;
        ret->next = product->components;
        product->components = ret;
        return ret;
    }
    return NULL;
}

void loki_remove_component(product_component_t *comp)
{
    product_option_t *opt, *nextopt;
    product_file_t   *scr, *nextscr;
	product_envvar_t *var, *nextvar;
    product_component_t *c, *prev = NULL;
    char script[PATH_MAX];

    xmlUnlinkNode(comp->node);
    xmlFreeNode(comp->node);
    free(comp->name);
    free(comp->version);
    free(comp->url);

    /* Free all options */
        
    opt = comp->options;
    while ( opt ) {
        product_file_t *file, *nextfile;
        nextopt = opt->next;
        
        file = opt->files;
        while ( file ) {
            nextfile = file->next;
            if ( file->type == LOKI_FILE_SCRIPT ) {
                snprintf(script, sizeof(script),"%s/.manifest/scripts/%s.sh", 
                         comp->product->info.root, file->path);
                unlink(script);
            }
            free(file->path);
            free(file);
            file = nextfile;
        }
        
        free(opt->name);
		if ( opt->tag )
			free(opt->tag);
        free(opt);
        opt = nextopt;
    }
    
    /* Free all scripts */
    scr = comp->scripts;
    while ( scr ){
        nextscr = scr->next;
        
        snprintf(script, sizeof(script),"%s/.manifest/scripts/%s.sh", comp->product->info.root,
                 scr->path);
        unlink(script);
        free(scr->path);
        free(scr);
        scr = nextscr;
    }

	/* Environment variables */
	var = comp->envvars;
	while ( var ) {
		nextvar = var->next;
		free(var->name);
		free(var->value);
		free(var);
		var = nextvar;
	}

    /* Remove this component from the linked list */
    for ( c = comp->product->components; c; c = c->next) {
        if ( c == comp ) {
            if ( prev ) {
                prev->next = comp->next;
            } else {
                comp->product->components = comp->next;
            }
        }
        prev = c;
    }
    
    comp->product->changed = 1;
    free(comp);
}

/* Set a specific URL for updates to that component */
void loki_seturl_component(product_component_t *comp, const char *url)
{
    xmlSetProp(comp->node, "update_url", url);
    free(comp->url);
    if ( url ) {
        comp->url = strdup(url);
    } else {
        comp->url = NULL;
    }
}

void loki_setversion_component(product_component_t *comp, const char *version)
{
    xmlSetProp(comp->node, "version", version);
    free(comp->version);
    comp->version = strdup(version);
    comp->product->changed = 1;
}

/* Get the URL for updates for a component; defaults to the product's URL if not defined */
const char *loki_geturl_component(product_component_t *comp)
{
    return comp->url ? comp->url : comp->product->info.url;
}

/* Enumerate options from components */

product_option_t *loki_getfirst_option(product_component_t *component)
{
    return component->options;
}

product_option_t *loki_getnext_option(product_option_t *opt)
{
    return opt ? opt->next : NULL;
}

product_option_t *loki_find_option(product_component_t *comp, const char *name)
{
    product_option_t *ret = comp->options;
    while ( ret ) {
        if ( !strcmp(ret->name, name) ) {
            return ret;
        }
        ret = ret->next;
    }
    return NULL;
}

const char *loki_getname_option(product_option_t *opt)
{
    return opt->name;
}

const char *loki_gettag_option(product_option_t *opt)
{
    return opt->tag;
}

size_t loki_getsize_option(product_option_t *opt)
{
    size_t size = 0;
    product_file_t *file;
    struct stat sb;

    for ( file = loki_getfirst_file(opt);
          file;
          file = loki_getnext_file(file) ) {
        if ( stat(loki_getpath_file(file), &sb) == 0 ) {
            size += sb.st_size;
        }
    }
    return size;
}

product_option_t *loki_create_option(product_component_t *component, const char *name, const char *tag)
{
    xmlNodePtr node = xmlNewChild(component->node, NULL, "option", NULL);
    if ( node ) {
        product_option_t *ret = (product_option_t *)malloc(sizeof(product_option_t));
        ret->node = node;
        ret->component = component;
        ret->name = strdup(name);
		ret->tag = tag ? strdup(tag) : NULL;
        ret->next = component->options;
        ret->files = NULL;
        component->options = ret;
        component->product->changed = 1;
        xmlSetProp(node, "name", name);
		if ( tag ) {
			xmlSetProp(node, "tag", tag);
		}
        return ret;
    }
    return NULL;
}

void loki_remove_option(product_option_t *opt)
{
    product_file_t *file, *nextfile;
    product_option_t *c, *prev = NULL;
    char script[PATH_MAX];

    xmlUnlinkNode(opt->node);
    xmlFreeNode(opt->node);
    free(opt->name);
	if ( opt->tag )
		free(opt->tag);

    file = opt->files;
    while ( file ) {
        nextfile = file->next;
        if ( file->type == LOKI_FILE_SCRIPT ) {
            snprintf(script, sizeof(script),"%s/.manifest/scripts/%s.sh", 
                     opt->component->product->info.root, file->path);
            unlink(script);
        }
        free(file->path);
        free(file);
        file = nextfile;
    }

    /* Remove this option from the linked list */
    for ( c = opt->component->options; c; c = c->next) {
        if ( c == opt ) {
            if ( prev ) {
                prev->next = opt->next;
            } else {
                opt->component->options = opt->next;
            }
        }
        prev = c;
    }

    opt->component->product->changed = 1;
    free(opt);
}

/* Enumerate files from options */

product_file_t *loki_getfirst_file(product_option_t *opt)
{
    return opt->files;
}

product_file_t *loki_getnext_file(product_file_t *file)
{
    return file ? file->next : NULL;
}

static product_file_t *find_file_by_name(product_option_t *opt, const char *path)
{
    product_file_t *file = opt->files;
    while ( file ) {
        if ( !strcmp(path, file->path) ) {
            return file;
        }
        file = file->next;
    }
    return NULL;
}

/* Get informations from a file */
file_type_t loki_gettype_file(product_file_t *file)
{
    return file->type;
}

/* This returns the expanded full path of the file if applicable */
const char *loki_getpath_file(product_file_t *file)
{
    if ( file->type == LOKI_FILE_RPM || file->type == LOKI_FILE_SCRIPT ) {
        return file->path;
    } else {
        static char buf[PATH_MAX]; /* FIXME: Evil static buffer */
        expand_path(file->option->component->product, file->path, buf, sizeof(buf));
        return buf;
    }
}

unsigned int loki_getmode_file(product_file_t *file)
{
    return file->mode;
}

/* Set the UNIX mode for the file */
void loki_setmode_file(product_file_t *file, unsigned int mode)
{
    char buf[20];
    file->mode = mode;
    snprintf(buf, sizeof(buf), "%04o", mode);
    xmlSetProp(file->node, "mode", buf);
    file->option->component->product->changed = 1;
}

/* Get / set the 'patched' attribute of a flag, i.e. it should not be removed unless
   the whole application is removed */
int loki_getpatched_file(product_file_t *file)
{
    return file->patched;
}

void loki_setpatched_file(product_file_t *file, int flag)
{
    file->patched = flag;
    xmlSetProp(file->node, "patched", flag ? "yes" : "no");
    file->option->component->product->changed = 1;
}

int loki_getmutable_file(product_file_t *file)
{
	return file->mutable;
}

void loki_setmutable_file(product_file_t *file, int flag)
{
    file->mutable = flag;
    xmlSetProp(file->node, "mutable", flag ? "yes" : "no");
    file->option->component->product->changed = 1;
}

product_option_t *loki_getoption_file(product_file_t *file)
{
    return file->option;
}

product_component_t *loki_getcomponent_file(product_file_t *file)
{
    return file->option->component;
}

product_t *loki_getproduct_file(product_file_t *file)
{
    return file->option->component->product;
}

unsigned char *loki_getmd5_file(product_file_t *file)
{
    return file->type==LOKI_FILE_REGULAR ? file->data.md5sum : NULL;
}

/* Enumerate all files of an option through a callback function */
int loki_enumerate_files(product_option_t *opt, product_file_cb cb)
{
    char buf[PATH_MAX];
    int count = 0;
    product_file_t *file = opt->files;

    while ( file ) {
        if ( file->type == LOKI_FILE_RPM || file->type == LOKI_FILE_SCRIPT ) {
            strncpy(buf, file->path, sizeof(buf));
        } else {
            expand_path(opt->component->product, file->path, buf, sizeof(buf));
        }
        cb(buf, file->type, opt->component, opt);
        count ++;
        file = file->next;
    }
    return count;
}

product_file_t *loki_findpath(const char *path, product_t *product)
{
    if ( product ) {
        product_component_t *comp;
        product_option_t *opt;
        product_file_t *file;

        for( comp = product->components; comp; comp = comp->next ) {
            for( opt = comp->options; opt; opt = opt->next ) {
                for( file = opt->files; file; file = file->next ) {
                    if ( file->type!=LOKI_FILE_SCRIPT && file->type!=LOKI_FILE_RPM ) {
                        if ( ! strcmp(loki_remove_root(product, path), file->path) ) {
                            /* We found our match */
                            return file;
                        }
                    }
                }
            }
        }
    } else {
        /* TODO: Try for each available product */
    }
    return NULL;
}

static product_file_t *registerfile_new(product_option_t *option, const char *path, const char *md5)
{
    struct stat st;
    char dev[10];
    char full[PATH_MAX];
    product_file_t *file;

    expand_path(option->component->product, path, full, sizeof(full));
    if ( lstat(full, &st) < 0 ) {
        return NULL;
    }
    file = (product_file_t *)malloc(sizeof(product_file_t));
    file->path = strdup(path);
	file->desktop = NULL;
    memset(file->data.md5sum, 0, 16);
    if ( S_ISREG(st.st_mode) ) {
        file->type = LOKI_FILE_REGULAR;
        if ( md5 ) {
            file->node = xmlNewChild(option->node, NULL, "file", substitute_xml_string(path));
            xmlSetProp(file->node, "md5", md5);
            memcpy(file->data.md5sum, get_md5_bin(md5), 16);
        } else {
            char md5sum[33];
            if ( *path == '/' ) {
                md5_compute(path, md5sum, 1);
            } else {
                char fpath[PATH_MAX];
                snprintf(fpath, sizeof(fpath), "%s/%s", option->component->product->info.root, path);
                md5_compute(fpath, md5sum, 1);
            }
            file->node = xmlNewChild(option->node, NULL, "file", substitute_xml_string(path));
            xmlSetProp(file->node, "md5", md5sum);
            memcpy(file->data.md5sum, get_md5_bin(md5sum), 16);
        }
    } else if ( S_ISDIR(st.st_mode) ) {
        file->type = LOKI_FILE_DIRECTORY;
        file->node = xmlNewChild(option->node, NULL, "directory", substitute_xml_string(path));
    } else if ( S_ISLNK(st.st_mode) ) {
        char buf[PATH_MAX];
        int count;

        file->type = LOKI_FILE_SYMLINK;
        file->node = xmlNewChild(option->node, NULL, "symlink", substitute_xml_string(path));
        count = readlink(full, buf, sizeof(buf));
        if ( count < 0 ) {
            fprintf(stderr, "readlink: Could not find symbolic link %s\n", full);
        } else {
            buf[count] = '\0';
            xmlSetProp(file->node, "dest", buf);
        }
    } else if ( S_ISFIFO(st.st_mode) ) {
        file->type = LOKI_FILE_FIFO;
        file->node = xmlNewChild(option->node, NULL, "fifo", substitute_xml_string(path));
    } else if ( S_ISBLK(st.st_mode) ) {
        file->type = LOKI_FILE_DEVICE;
        file->node = xmlNewChild(option->node, NULL, "device", substitute_xml_string(path));
        xmlSetProp(file->node, "type", "block");
        /* Get the major/minor device number info */
        snprintf(dev,sizeof(dev),"%d", major(st.st_rdev));
        xmlSetProp(file->node, "major", dev);
        snprintf(dev,sizeof(dev),"%d", minor(st.st_rdev));
        xmlSetProp(file->node, "minor", dev);
    } else if ( S_ISCHR(st.st_mode) ) {
        file->type = LOKI_FILE_DEVICE;
        file->node = xmlNewChild(option->node, NULL, "device", substitute_xml_string(path));
        xmlSetProp(file->node, "type", "char");
        /* Get the major/minor device number info */
        snprintf(dev,sizeof(dev),"%d", major(st.st_rdev));
        xmlSetProp(file->node, "major", dev);
        snprintf(dev,sizeof(dev),"%d", minor(st.st_rdev));
        xmlSetProp(file->node, "minor", dev);
    } else {
        file->node = NULL;
        file->type = LOKI_FILE_NONE;
        /* TODO: Warning? */
        return NULL;
    }
    file->patched = 0;
	/* Get the actual mode from the file */
    file->mode = (st.st_mode & 07777);
	snprintf(dev, sizeof(dev), "%04o", file->mode);
    xmlSetProp(file->node, "mode", dev);
    file->option = option;
    insert_end_file(file, &option->files);

    option->component->product->changed = 1;
    return file;
}

static product_file_t *registerfile_update(product_option_t *option, product_file_t *file,
                                           const char *md5)
{
    char buf[PATH_MAX];
    int count;
    unsigned char *md5bin;

    switch(file->type) {
    case LOKI_FILE_REGULAR:
        /* Compare MD5 checksums; if different then the 'patched' attribute is set automatically */
        if ( md5 ) {
            md5bin = get_md5_bin(md5);
            xmlSetProp(file->node, "md5", md5);
            if ( memcmp(file->data.md5sum, md5bin, 16) ) {
                loki_setpatched_file(file, 1);
            }
            memcpy(file->data.md5sum, md5bin, 16);
        } else {
            char md5sum[33];
            if ( *file->path == '/' ) {
                md5_compute(file->path, md5sum, 1);
            } else {
                snprintf(buf, sizeof(buf), "%s/%s", option->component->product->info.root, file->path);
                md5_compute(buf, md5sum, 1);
            }
            xmlSetProp(file->node, "md5", md5sum);
            md5bin = get_md5_bin(md5sum);
            if ( memcmp(file->data.md5sum, md5bin, 16) ) {
                loki_setpatched_file(file, 1);
            }
            memcpy(file->data.md5sum, md5bin, 16);
        }
        option->component->product->changed = 1;
        break;
    case LOKI_FILE_SYMLINK:
        if ( *file->path == '/' ) {
            count = readlink(file->path, buf, sizeof(buf));
            if ( count < 0 ) {
                fprintf(stderr, "Couldn't read link: %s\n", file->path);
            }
        } else {
            char path[PATH_MAX];
            snprintf(path, sizeof(path), "%s/%s", option->component->product->info.root, file->path);
            count = readlink(path, buf, sizeof(buf));
            if ( count < 0 ) {
                fprintf(stderr, "Couldn't read link: %s\n", path);
            }
        }
        if ( count >= 0 ) {
            buf[count] = '\0';
            xmlSetProp(file->node, "dest", buf);
        }   
        option->component->product->changed = 1;
        break;

    case LOKI_FILE_DIRECTORY:
    case LOKI_FILE_DEVICE:
    case LOKI_FILE_SOCKET:
    case LOKI_FILE_FIFO:
    case LOKI_FILE_RPM:
    case LOKI_FILE_SCRIPT:
    case LOKI_FILE_NONE:
        /* We don't need to update the MD5 checksums of these elements */
        break;
    }
    return file;
}

/* Register a file, returns 0 if OK */
product_file_t *loki_register_file(product_option_t *option, const char *path, const char *md5)
{
    const char *nrpath = loki_remove_root(option->component->product, path);
    product_file_t *file;

	if ( ! *nrpath )  /* Trying to register the root path */
		return NULL;

    /* Check if this file is already registered for this option */
    file = find_file_by_name(option, nrpath);
    if ( file ) {
        return registerfile_update(option, file, md5);
    } else {
        file = loki_findpath(path, option->component->product);
        if ( file ) {
            loki_unregister_file(file);
        }
        return registerfile_new(option, nrpath, md5);
    }
}

/* Indicate that a file is a desktop item for a binary */
int loki_setdesktop_file(product_file_t *file, const char *binary)
{
	if ( file && binary ) {
		if ( file->desktop )
			free(file->desktop);
		file->desktop = strdup(binary);
		xmlSetProp(file->node, "desktop", binary);
		file->option->component->product->changed = 1;
		return 1;
	}
	return 0;
}

/* Test whether a file is a desktop item, optionally if it matches a binary */
int loki_isdesktop_file(product_file_t *file, const char *binary)
{
	if ( file ) {
		if ( binary ) {
			return file->desktop && !strcmp(file->desktop, binary);
		} else {
			return file->desktop != NULL;
		}
	}
	return 0;
}

/* Check a file against its MD5 checksum, for integrity */
file_check_t loki_check_file(product_file_t *file)
{
	char path[PATH_MAX];
	char md5sum[33];
	char *str;
	struct stat st;
	file_check_t ret = LOKI_OK;

	expand_path(file->option->component->product, file->path, path, sizeof(path));

    switch(file->type) {
    case LOKI_FILE_REGULAR:
		if ( access(path, F_OK) < 0 )
			return LOKI_REMOVED;
		if ( file->mutable )
			return LOKI_OK;

		/* Compare MD5 checksums if file exists */
		md5_compute(path, md5sum, 1);
		str = xmlGetProp(file->node, "md5");
		if ( str && strncmp(md5sum, str, CHECKSUM_SIZE) ) {
			ret = LOKI_CHANGED;
		}
		xmlFree(str);
		break;
    case LOKI_FILE_SYMLINK:
		if ( lstat(path, &st) < 0 )
			return LOKI_REMOVED;
		if ( file->mutable )
			return LOKI_OK;
		/*  Compare symlinks contents */
		str = xmlGetProp(file->node, "dest");
		if ( str ) {
			char buf[PATH_MAX];
			int count;

			count = readlink(path, buf, sizeof(buf));
			if ( count < 0 ) {
				xmlFree(str);
				return LOKI_CHANGED;
			} else {
				buf[count] = '\0';
			}
			if ( strcmp(buf, str) )
				ret = LOKI_CHANGED;
			xmlFree(str);
		}
		break;
    case LOKI_FILE_DEVICE:
		if ( stat(path, &st) < 0 )
			return LOKI_REMOVED;
		/* Check that device has the same characteristics */
		str = xmlGetProp(file->node, "type");
		if ( str ) {
			if ( !strcmp(str, "block") && !S_ISBLK(st.st_mode) ) {
				ret =  LOKI_CHANGED;
			} else if ( !strcmp(str, "char") && !S_ISCHR(st.st_mode) ) {
				ret = LOKI_CHANGED;
			} else {
				xmlFree(str);
				str = xmlGetProp(file->node, "major");
				if ( str && major(st.st_rdev)!=atoi(str) ) {
					ret = LOKI_CHANGED;
				} else {
					xmlFree(str);
					str = xmlGetProp(file->node, "minor");
					if ( str && minor(st.st_rdev)!=atoi(str) )
						ret = LOKI_CHANGED;
				}
			}
			xmlFree(str);
		}
		break;
    case LOKI_FILE_DIRECTORY:
    case LOKI_FILE_SOCKET:
    case LOKI_FILE_FIFO:
		/* FIXME: Only test for existence */
		if ( access(path, F_OK) < 0 )
			return LOKI_REMOVED;
		break;
    case LOKI_FILE_RPM:
    case LOKI_FILE_SCRIPT:
    case LOKI_FILE_NONE:
		/* Do nothing */
		break;
	}
	return ret;
}

static void unregister_file(product_file_t *file, product_file_t **opt)
{
    xmlUnlinkNode(file->node);
    xmlFreeNode(file->node);
    /* Remove the file from the list */
    if ( *opt == file ) {
        *opt = file->next;
    } else {
        product_file_t *f;
        for(f = *opt; f; f = f->next) {
            if (f->next == file ) {
                f->next = file->next;
                break;
            }
        }
    }
    free(file->path);
    free(file);
}

/* Remove a file from the registry. */
int loki_unregister_path(product_option_t *option, const char *path)
{
    product_file_t *file = find_file_by_name(option,
                                             loki_remove_root(option->component->product,path));
    if ( file ) {
        unregister_file(file, &option->files);
        option->component->product->changed = 1;
        return 0;
    }
    return -1;
}

int loki_unregister_file(product_file_t *file)
{
    if ( file ) {
        product_option_t *option = file->option;
        if ( option ) { /* Does not work for scripts anyway */
            unregister_file(file, &option->files);
            option->component->product->changed = 1;
            return 0;
        }
    }
    return -1;
}

/* Register a new RPM as having been installed by this product */
int loki_register_rpm(product_option_t *option, const char *name, const char *version, int revision,
                     int autoremove)
{
    product_file_t *rpm;
    char rev[10];

    rpm = (product_file_t *) malloc(sizeof(product_file_t));
    rpm->node = xmlNewChild(option->node, NULL, "rpm", substitute_xml_string(name));
    xmlSetProp(rpm->node, "version", version);
    snprintf(rev, sizeof(rev), "%d", revision);
    xmlSetProp(rpm->node, "revision", rev);
    xmlSetProp(rpm->node, "autoremove", autoremove ? "yes" : "no");

    rpm->option = option;
    rpm->path = strdup(name);
    rpm->type = LOKI_FILE_RPM;
	rpm->desktop = NULL;
    rpm->next = option->files;
    option->files = rpm;
    option->component->product->changed = 1;

    return 0;
}

int loki_unregister_rpm(product_t *product, const char *name)
{
    /* TODO: Search for a match */

    product->changed = 1;

    return -1;
}

static product_file_t *registerscript(xmlNodePtr parent, script_type_t type, const char *name,
                          const char *script, product_t *product)
{
    char buf[PATH_MAX];
    FILE *fd;
    
    snprintf(buf, sizeof(buf), "%s/.manifest/scripts/%s.sh", product->info.root, name);
    fd = fopen(buf, "w");
    if (fd) {
        product_file_t *scr;
        scr = (product_file_t *) malloc(sizeof(product_file_t));

        fprintf(fd, "#! /bin/sh\n");
        fprintf(fd, "%s", script);
        fchmod(fileno(fd), 0755);
        fclose(fd);

        scr->node = xmlNewChild(parent, NULL, "script", substitute_xml_string(name));
        xmlSetProp(scr->node, "type", script_types[type]);
        product->changed = 1;

        scr->path = strdup(name);
        scr->type = LOKI_FILE_SCRIPT;
        scr->data.scr_type = type;
		scr->desktop = NULL;
        return scr;
    }
    return NULL;
}

/* Register a new script for the component. 'script' holds the whole script in one string */
int loki_registerscript_component(product_component_t *comp, script_type_t type, const char *name,
                                  const char *script)
{
    product_file_t *file = registerscript(comp->node, type, name, script, comp->product);
    if ( file ) {
        file->option = NULL;
        file->next = comp->scripts;
        comp->scripts = file;
        return 0;
    }
    return -1;
}

int loki_registerscript(product_option_t *opt, script_type_t type, const char *name,
                                  const char *script)
{
    product_file_t *file = registerscript(opt->node, type, name, script, opt->component->product);
    if ( file ) {
        file->option = opt;
        file->next = opt->files;
        opt->files = file;
        return 0;
    }
    return -1;
}

int loki_registerscript_fromfile_component(product_component_t *comp, script_type_t type,
                                           const char *name, const char *path)
{
    struct stat st;
    int ret = -1;

    if ( !stat(path, &st) ) {
        FILE *fd;
        char *script = (char *)malloc(st.st_size+1);

        fd = fopen(path, "r");
        if ( fd ) {
            product_file_t *file;
            fread(script, st.st_size, 1, fd);
            fclose(fd);
            script[st.st_size] = '\0';
            file = registerscript(comp->node, type, name, script, comp->product);
            if ( file ) {
                file->option = NULL;
                file->next = comp->scripts;
                comp->scripts = file;
                ret = 0;
            }
        }
        free(script);
    }
    return ret;
}

int loki_registerscript_fromfile(product_option_t *opt, script_type_t type,
                                 const char *name, const char *path)
{
    struct stat st;
    int ret = -1;

    if ( !stat(path, &st) ) {
        FILE *fd;
        char *script = (char *)malloc(st.st_size+1);

        fd = fopen(path, "r");
        if ( fd ) {
            product_file_t *file;
            fread(script, st.st_size, 1, fd);
            fclose(fd);
            script[st.st_size] = '\0';
            file = registerscript(opt->node, type, name, script, opt->component->product);
            if ( file ) {
                file->option = opt;
                file->next = opt->files;
                opt->files = file;
                ret = 0;
            }
        }
        free(script);
    }
    return ret;
}

/* Unregister a registered script, and remove the file */
int loki_unregister_script(product_component_t *comp, const char *name)
{
    char buf[PATH_MAX];
    int ret = 0;
    product_file_t *file;
    product_option_t *opt;

    snprintf(buf, sizeof(buf), "%s/.manifest/scripts/%s.sh", comp->product->info.root, name);

    if ( !access(buf, W_OK) && unlink(buf)<0 ) {
        perror("unlink");
        ret ++;
    }

    /* First look at global scripts */
    for ( file = comp->scripts; file; file = file->next ) {
        if( !strcmp(file->path, name) ) {
            unregister_file(file, &comp->scripts);
            comp->product->changed = 1;
            return ret;
        }
    }

    for ( opt = comp->options; opt; opt = opt->next ) {
        for ( file = opt->files; file; file = file->next ) {
            if( !strcmp(file->path, name) ) {
                unregister_file(file, &opt->files);
                comp->product->changed = 1;
                return ret;
            }
        }
    }

    return ++ret;
}

static int register_envvar(product_t *product, product_envvar_t **vars, const char *name)
{
	product_envvar_t *var;
	const char *env = getenv(name);

	if ( ! env ) {
		return 1; /* Do not fail, but don't store it */
	}

	/* Look for an existing record */
	for(var = *vars; var; var = var->next ) {
		if ( !strcmp(var->name, name) ) {
			if ( !strcmp(var->value, env) )
				return 1; /* Nothing to do ! */
			break;
		}
	}

	if ( var ) {
		free(var->value);
		var->value = strdup(env); /* Update the value */		
	} else {
		var = malloc(sizeof(product_envvar_t));
		if ( !var )
			return 0;
		var->name = strdup(name);
		var->value = strdup(env);
		var->node = xmlNewChild(product->doc->root, NULL, "environment", NULL);

		var->next = *vars;
		*vars = var;
	}

	xmlSetProp(var->node, "var", name);
	xmlSetProp(var->node, "value", env);

	product->changed = 1;
	return 1;
}

static int unregister_envvar(product_t *product, product_envvar_t **vars, const char *name)
{
	product_envvar_t *var, *prev = NULL;

	for(var = *vars; var; var = var->next ) {
		if ( !strcmp(var->name, name) ) {
			xmlUnlinkNode(var->node);
			xmlFreeNode(var->node);

			if ( prev ) {
				prev->next = var->next;
			} else {
				*vars = var->next;
			}
			free(var->name);
			free(var->value);
			free(var);
			product->changed = 1;
			return 1;
		}
		prev = var;
	}

	return 0;
}

/* Environment variables management */
int loki_register_envvar(product_t *product, const char *name)
{
	return register_envvar(product, &product->envvars, name);
}

int loki_register_envvar_component(product_component_t *comp, const char *name)
{
	return register_envvar(comp->product, &comp->envvars, name);
}

int loki_unregister_envvar(product_t *product, const char *name)
{
	return unregister_envvar(product, &product->envvars, name);
}

int loki_unregister_envvar_component(product_component_t *comp, const char *name)
{
	return unregister_envvar(comp->product, &comp->envvars, name);
}


/* Put the registered environment variables in the environment,
   returns number of variables affected */
int loki_put_envvars(product_t *product)
{
	product_envvar_t *var;
	int count = 0;

	for(var = product->envvars; var; var = var->next ) {
#ifdef HAVE_SETENV
		if ( !setenv(var->name, var->value, 1) )
			count ++;
#else
		char buf[PATH_MAX];
		snprintf(buf, sizeof(buf), "%s=%s", var->name, var->value);
		if ( !putenv(strdup(buf)) ) /* Intentional memory leak */
			count ++;
#endif		
	}
	return count;
}

/* Put the registered environment variables in the environment,
   returns number of variables affected.
   Global product variables are set first, and then any component-specific variables.
*/
int loki_put_envvars_component(product_component_t *comp)
{
	product_envvar_t *var;
	int count = loki_put_envvars(comp->product);

	for(var = comp->envvars; var; var = var->next ) {
#ifdef HAVE_SETENV
		if ( !setenv(var->name, var->value, 1) )
			count ++;
#else
		char buf[PATH_MAX];
		snprintf(buf, sizeof(buf), "%s=%s", var->name, var->value);
		if ( !putenv(strdup(buf)) ) /* Intentional memory leak */
			count ++;
#endif		
	}
	return count;
}

/* Returns the error code from the command */
static int run_script(product_t *prod, const char *name)
{
    char cmd[PATH_MAX];
	int ret = 1;
    
    if ( name ) {
        snprintf(cmd, sizeof(cmd), "\"%s/.manifest/scripts/%s.sh\"", prod->info.root, name);
        ret = system(cmd);
		if ( ret != -1 )
			ret = WEXITSTATUS(ret);
    }
	return ret;
}

/* Run all scripts of a given type, returns the number of scripts successfully run, or -1 if one of them failed */
int loki_runscripts(product_component_t *comp, script_type_t type)
{
    int count = 0, maj = 0, min = 0, ret;
    product_file_t *file;
    product_option_t *opt;
	distribution distro = detect_distro(&maj, &min);

#ifdef HAVE_SETENV
	setenv("SETUP_INSTALLPATH", comp->product->info.root, 1);
	setenv("SETUP_PRODUCTNAME", comp->product->info.name, 1);
	setenv("SETUP_DISTRO", distribution_symbol[distro], 1);
#else
	static char buf1[PATH_MAX], buf2[PATH_MAX], buf3[PATH_MAX];

	snprintf(buf1, sizeof(buf1), "SETUP_INSTALLPATH=%s", comp->product->info.root);
	putenv(buf1);
	snprintf(buf2, sizeof(buf2), "SETUP_PRODUCTNAME=%s", comp->product->info.name);
	putenv(buf2);
	snprintf(buf3, sizeof(buf3), "SETUP_DISTRO=%s", distribution_symbol[distro]);
	putenv(buf3);
#endif

    /* First look at component-wide scripts */
    for ( file = comp->scripts; file; file = file->next ) {
        if( file->data.scr_type == type ) {
            ret = run_script(comp->product, file->path);
			if ( ret == 0 )
				count ++;
			else
				return -1;
        }
    }

    for ( opt = comp->options; opt; opt = opt->next ) {
        for ( file = opt->files; file; file = file->next ) {
            if( file->type==LOKI_FILE_SCRIPT && file->data.scr_type==type ) {
                ret = run_script(comp->product, file->path);
				if ( ret == 0 )
					count ++;
				else
					return -1;
            }
        }
    }

    return count;
}

/* This copies a binary that might be from a CD mounted with noexec attributes to 
   a temporary place where we are sure to be able to run it */
static const char *copy_binary_to_tmp(const char *path)
{
    int dst, src;
    static char tmppath[PATH_MAX];
    struct stat st;
    void *buffer;
	const char *env = getenv("TMPDIR");

	if ( env ) {
		snprintf(tmppath, sizeof(tmppath), "%s/setupdb-bin.XXXXXX", env);
	} else {
		strcpy(tmppath, "/tmp/setupdb-bin.XXXXXX");
	}

    dst = mkstemp(tmppath);
    if ( dst < 0 ) {
        perror("mkstemp");
        return NULL;
    }

    src = open(path, O_RDONLY);
    if ( src < 0 ) {
        perror("open");
        return NULL;
    }

    fstat(src, &st);
    buffer = malloc(st.st_size);

    read(src, buffer, st.st_size);
    write(dst, buffer, st.st_size);

    free(buffer);
    close(src);
    fchmod(dst, 0755);
    close(dst);

    return tmppath;
}

/* Perform automatic update of the uninstaller binaries and script.
   'src' is the path where updated binaries can be copied from.
 */
int loki_upgrade_uninstall(product_t *product, const char *src_bins, const char *locale_path)
{
    char binpath[PATH_MAX];
    const char *os_name = detect_os();
    int perform_upgrade = 0;
    product_info_t *pinfo;
    FILE *scr;
	FILE *src, *dst;
	const char *lang;
	void *data;
	struct stat st;


    /* TODO: Locate global loki-uninstall and upgrade it if we have sufficient permissions */    

    snprintf(binpath, sizeof(binpath), "%s/" LOKI_DIRNAME "/installed/locale", detect_home());
    mkdir(binpath, 0755);

    snprintf(binpath, sizeof(binpath), "%s/" LOKI_DIRNAME "/installed/bin", detect_home());
    mkdir(binpath, 0755);

    strncat(binpath, "/", sizeof(binpath));
    strncat(binpath, os_name, sizeof(binpath));
    mkdir(binpath, 0755);

    strncat(binpath, "/", sizeof(binpath));
    strncat(binpath, detect_arch(), sizeof(binpath));
    mkdir(binpath, 0755);

    strncat(binpath, "/uninstall", sizeof(binpath));

    if ( !access(binpath, X_OK) && !access(src_bins, R_OK) ) {
        char cmd[PATH_MAX];
        FILE *pipe;
        
        snprintf(cmd, sizeof(cmd), "%s -v", binpath);
        pipe = popen(cmd, "r");
        if ( pipe ) {
            int major, minor, rel, ret;
            const char *tmpbin;

            /* Try to see if we have to update it */
            ret = fscanf(pipe, "%d.%d.%d", &major, &minor, &rel);
            pclose(pipe);

            if ( ret==EOF || ret<3 )
                perform_upgrade = 1;

            /* Now check against the version of the binaries we have */
            tmpbin = copy_binary_to_tmp(src_bins);
            if ( tmpbin ) {
                snprintf(cmd, sizeof(cmd), "%s -v", tmpbin);
            } else { /* We still try to run it if the copy failed for some reason */
                snprintf(cmd, sizeof(cmd), "%s -v", src_bins);
            }
            pipe = popen(cmd, "r");
            if ( pipe ) {
                int our_maj, our_min, our_rel;
                fscanf(pipe, "%d.%d.%d", &our_maj, &our_min, &our_rel);
                pclose(pipe);
                if ( (major < our_maj) || 
                     ((major==our_maj) && (minor < our_min)) ||
                     ((major==our_maj) && (minor == our_min) &&
                      (rel < our_rel )) ) {
                    /* Perform the upgrade, overwrite the uninstall binary */
                    perform_upgrade = 1;
                }
            }
            if ( tmpbin ) { /* Remove the copied binary */
                unlink(tmpbin);
            }
        } else {
            perror("popen");
        }
    } else {
        perform_upgrade = 1;
    }

    /* TODO: Try to install global command in the symlinks path */

    if ( perform_upgrade ) {
		stat(src_bins, &st);
        src = fopen(src_bins, "rb");
        if ( src ) {
            dst = fopen(binpath, "wb");
            if ( dst ) {
                data = malloc(st.st_size);
                fread(data, 1, st.st_size, src);
                fwrite(data, 1, st.st_size, dst);
                free(data);
                fchmod(fileno(dst), 0755);
                fclose(dst);                
            } else {
                fprintf(stderr, "Couldn't write to %s!\n", binpath);
            }
            fclose(src);
        } else {
            fprintf(stderr, "Couldn't open %s to be copied!\n", src_bins);
        }
    }

	
	/* Copy the locale files always */
	lang = getenv("LC_ALL");
	if ( ! lang )
		lang = getenv("LC_MESSAGES");
	if ( ! lang ) 
		lang = getenv("LANG");
	if ( lang && locale_path ) {
		int found = 0;
            
		/* Locate a MO file we can copy */
		snprintf(binpath, sizeof(binpath), "%s/%.5s/LC_MESSAGES/loki-uninstall.mo", 
				 locale_path, lang);
		if ( access(binpath, R_OK) ) {
			snprintf(binpath, sizeof(binpath), "%s/%.2s/LC_MESSAGES/loki-uninstall.mo", 
					 locale_path, lang);
			if ( !access(binpath, R_OK) )
				found = 1;
		} else {
			found = 1;
		}

		if ( found ) {
			stat(binpath, &st);
			src = fopen(binpath, "rb");
			if ( src ) {
				/* Start by creating the directories */
				snprintf(binpath, sizeof(binpath), "%s/" LOKI_DIRNAME "/installed/locale/%.5s", detect_home(),
						 lang);
				mkdir(binpath, 0755);
				
				strncat(binpath, "/LC_MESSAGES", sizeof(binpath));
				mkdir(binpath, 0755);

				snprintf(binpath, sizeof(binpath),
						 "%s/" LOKI_DIRNAME "/installed/locale/%.5s/LC_MESSAGES/loki-uninstall.mo",
						 detect_home(), lang);
				dst = fopen(binpath, "wb");
				if ( dst ) {
					data = malloc(st.st_size);
					fread(data, 1, st.st_size, src);
					fwrite(data, 1, st.st_size, dst);
					free(data);
					fclose(dst);
				}
				fclose(src);
			}
		}
	}

    /* Now we create an 'uninstall' shell script in the game's installation directory */
    pinfo = loki_getinfo_product(product);
    snprintf(binpath, sizeof(binpath), "%s/uninstall", pinfo->root);
    scr = fopen(binpath, "w");
    if ( scr ) {
        fprintf(scr,
                "#! /bin/sh\n"
                "#### UNINSTALL SCRIPT - Generated by SetupDB %d.%d #####\n"
                "DetectARCH()\n"
                "{\n"
                "        status=1\n"
                "        case `uname -m` in\n"
                "           amd64 | x86_64)  echo \"amd64\"\n"
                "                  status=0;;\n"
                "           i?86 | i86*)  echo \"x86\"\n"
                "                  status=0;;\n"
				"           90*/*)\n"
				"		   echo \"hppa\"\n"
				"		   status=0;;\n"
				"	    *)\n"
				"		case `uname -s` in\n"
				"		    IRIX*)\n"
				"			echo \"mips\"\n"
				"			status=0;;\n"
				"           AIX*)\n"
				"           echo \"ppc\"\n"
				"           status=0;;\n"
				"		    *)\n"
				"			arch=`uname -p 2>/dev/null || uname -m`\n"
				"                       if test \"$arch\" = powerpc; then\n"
				"                          echo \"ppc\"\n"
				"                       else\n"
				"                          echo $arch\n"
				"                       fi\n"
				"			status=0;;\n"
				"		esac\n"
				"        esac\n"
                "        return $status\n"
                "}\n\n"

				"DetectOS()\n"
				"{\n"
				"  os=`uname -s`\n"
				"  if test \"$os\" = OpenUNIX; then\n"
				"     echo SCO_SV\n"
				"  else\n"
				"     echo $os\n"
				"  fi\n"
				"  return 0\n"
				"}\n\n"

                "if which " LOKI_PREFIX "-uninstall 2> /dev/null > /dev/null || type -p "
				LOKI_PREFIX "-uninstall 2> /dev/null > /dev/null; then\n"
                "    UNINSTALL=" LOKI_PREFIX "-uninstall\n"
                "else\n"
                "    UNINSTALL=\"$HOME/" LOKI_DIRNAME "/installed/bin/`DetectOS`/`DetectARCH`/uninstall\"\n"
                "    if test ! -x \"$UNINSTALL\" ; then\n"
                "        echo Could not find a usable uninstall program. Aborting.\n"
                "        exit 1\n"
                "    fi\n"
                "fi\n"
                "\"$UNINSTALL\" -L %s \"%s\" \"$1\"",
                SETUPDB_VERSION_MAJOR, SETUPDB_VERSION_MINOR,
				pinfo->name,
                pinfo->registry_path);
        fchmod(fileno(scr), 0755);
        fclose(scr);
    } else {
        fprintf(stderr, "Could not write uninstall script: %s\n", binpath);
    }
    return 0;
}
