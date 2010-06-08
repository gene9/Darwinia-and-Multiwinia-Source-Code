/* Command-line utility to manipulate product entries from scripts */

/* $Id: register.c,v 1.14 2005/02/05 02:05:34 megastep Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "setupdb.h"

product_t *product;

void print_usage(const char *argv0)
{
    printf("Usage: %s <product> [command] [args]\n"
			"Recognized commands are :\n"
			"   create <component> <version> [option_name [option_tag]]\n"
			"      Create a new component and/or option in the component\n"
			"   add <component> <option> [file [file ...]]\n"
			"      Register files in the component / option\n"
			"   script <component> <pre|post> <name> <path-to-script>\n"
			"      Register a new pre/post-uninstall script for the component\n"
			"   update <component> <option> <files>\n"
			"      Updates registration information\n"
			"   message <component> <\"message\">\n"
			"      Add an uninstallation warning message to the component\n"
			"   remove file [file [file ...]]\n"
			"      Remove specified files from the product\n"
			"   listfiles [component]\n"
			"      List files installed under product [or component]\n"
		    "   desktop <component> <binary>\n"
		    "      List all desktop items installed for a binary\n"
			"   printtags [component]\n"
			"      Print installed option tags\n",
           argv0);
}

/* Create - does not update the version or tags ! */
int create_option(const char *component, const char *ver, const char *option_name, const char *option_tag)
{
    product_component_t *comp;
    product_option_t *opt;

    comp = loki_find_component(product, component);
    if ( ! comp ) {
		comp = loki_create_component(product, component, ver);
		if ( ! comp ) {
			fprintf(stderr, "Failed to create component '%s' version '%s'\n", component, ver);
			return 1;
		}
	}
	if ( option_name ) {
		opt = loki_find_option(comp, option_name);
		if ( ! opt ) {
			opt = loki_create_option(comp, option_name, option_tag);
			if ( ! opt ) {
				fprintf(stderr, "Failed to create option '%s' in component '%s'\n", option_name, component);
				return 1;
			}
		}
	}
	return 0;
}

int printtags(const char *component)
{
    product_component_t *comp;
    product_option_t *opt;
	const char *tag;

	if ( component ) {
		comp = loki_find_component(product, component);
		if ( ! comp ) {
			fprintf(stderr,"Unable to find component %s !\n", component);
			return 1;
		}
		for ( opt = loki_getfirst_option(comp); opt; opt = loki_getnext_option(opt) ) {
			tag = loki_gettag_option(opt);
			if ( tag ) {
				printf("%s ", tag);
			}
		}
	} else { /* Do all components */
		for ( comp = loki_getfirst_component(product); comp; comp = loki_getnext_component(comp) ) {
			for ( opt = loki_getfirst_option(comp); opt; opt = loki_getnext_option(opt) ) {
				tag = loki_gettag_option(opt);
				if ( tag ) {
					printf("%s ", tag);
				}
			}
		}
	}
	return 0;
}

int add_message(const char *component, const char *msg)
{
    product_component_t *comp;
	if ( component ) {
		comp = loki_find_component(product, component);
		if ( ! comp ) {
			fprintf(stderr,"Unable to find component %s !\n", component);
			return 1;
		}
		loki_setmessage_component(comp, msg);
	}
	return 0;
}

int register_script(const char *component, script_type_t type, const char *name, const char *script)
{
    product_component_t *comp;

    comp = loki_find_component(product, component);
    if ( ! comp ) {
        fprintf(stderr,"Unable to find component %s !\n", component);
        return 1;
    }

	if ( loki_registerscript_fromfile_component(comp, type, name, script) < 0 ){
		fprintf(stderr,"Error trying to register script for component %s !\n", component);
		return 1;
	}
	return 0;
}

int register_files(const char *component, const char *option, char **files)
{
    product_component_t *comp;
    product_option_t *opt;

    comp = loki_find_component(product, component);
    if ( ! comp ) {
        fprintf(stderr,"Unable to find component %s !\n", component);
        return 1;
    }
    opt = loki_find_option(comp, option);
    if ( ! opt ) {
        fprintf(stderr,"Unable to find option %s !\n", option);
        return 1;
    }

    for ( ; *files; files ++ ) {
        if ( ! loki_register_file(opt, *files, NULL) ) {
            fprintf(stderr,"Error while registering %s\n", *files);
            return 1;
        }
    }

    return 0;
}

int remove_files(char **files)
{
    for ( ; *files; files ++ ) {
        if ( loki_unregister_file(loki_findpath(*files, product)) < 0) {
            fprintf(stderr,"Error while removing %s\n", *files);
        }
    }
    return 0;
}

int list_files(const char *component)
{
	product_component_t *comp;
	product_file_t *file;
	product_option_t *option;

	/* If component_name is NULL, then we assume they want everything
	so we recurse over all components */

	if(component == NULL) {
		int return_val = 0;
		for ( comp = loki_getfirst_component(product); comp;
				comp = loki_getnext_component(comp) ) {
			if(list_files(loki_getname_component(comp)) > 0) {
				return_val = 1;
			}
		}
		return return_val;
	}

	comp = loki_find_component(product, component);
	if(comp == NULL) {
		fprintf(stderr,"Unable to find component %s !\n", component);
		return 1;
	}

	/* We've found the component we want details on */

	printf("Component: %s\n\n",component);

	for ( option = loki_getfirst_option(comp); option;
			option = loki_getnext_option(option) ) {
		printf("  Option: %s\n", loki_getname_option(option));
		for( file = loki_getfirst_file(option); file;
			file = loki_getnext_file(file) ) {

			switch( loki_check_file(file) ) {
				case LOKI_REMOVED:
					printf("    %s REMOVED\n",loki_getpath_file(file));
					break;
				case LOKI_CHANGED:
				case LOKI_OK:
				default:
					printf("    %s\n",loki_getpath_file(file));
					break;
			}
		}
		printf("\n");
	}
	printf("\n");
	return 0;
}

int list_desktop(const char *component, const char *binary)
{
	product_component_t *comp;
	product_file_t *file;
	product_option_t *option;

	comp = loki_find_component(product, component);
	if(comp == NULL) {
		fprintf(stderr,"Unable to find component %s !\n", component);
		return 1;
	}

	for ( option = loki_getfirst_option(comp); option;
			option = loki_getnext_option(option) ) {
		for( file = loki_getfirst_file(option); file;
			file = loki_getnext_file(file) ) {

			if( loki_isdesktop_file(file, binary) ) {
				puts(loki_getpath_file(file));
			}
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	int ret = 1;
    if ( argc < 3 ) {
        print_usage(argv[0]);
        return 1;
    }

    product = loki_openproduct(argv[1]);
    if ( ! product ) {
        fprintf(stderr,"Unable to open product %s\n", argv[1]);
        return 1;
    }
    if ( !strcmp(argv[2], "add") || !strcmp(argv[2], "update")) {
        if ( argc < 6 ) {
            print_usage(argv[0]);
        } else {
            ret = register_files(argv[3], argv[4], &argv[5]);
        }
    } else if ( !strcmp(argv[2], "remove") ) {
        if ( argc < 4 ) {
            print_usage(argv[0]);
        } else {
            ret = remove_files(&argv[3]);
        }
    } else if ( !strcmp(argv[2], "message") ) {
        if ( argc != 5 ) {
            print_usage(argv[0]);
        } else {
            ret = add_message(argv[3], argv[4]);
        }
    } else if ( !strcmp(argv[2], "create") ) {
        if ( argc < 5 ) {
            print_usage(argv[0]);
        } else {
			ret = create_option(argv[3], argv[4], argc>5 ? argv[5] : NULL, argc>6 ? argv[6] : NULL);
		}
	} else if ( !strcmp(argv[2], "script") ) {
        if ( argc < 7 ) {
            print_usage(argv[0]);
        } else {
			if ( !strcmp(argv[4], "pre") ) {
				ret = register_script(argv[3], LOKI_SCRIPT_PREUNINSTALL, argv[5], argv[6]);
			} else if ( !strcmp(argv[4], "post") ) {
				ret = register_script(argv[3], LOKI_SCRIPT_POSTUNINSTALL, argv[5], argv[6]);
			} else {
				print_usage(argv[0]);
			}
		}
    } else if ( !strcmp(argv[2], "listfiles") ) {
		ret = list_files(argc>3 ? argv[3] : NULL);
    } else if ( !strcmp(argv[2], "desktop") ) {
        if ( argc != 5 ) {
            print_usage(argv[0]);
        } else {
			ret = list_desktop(argv[3], argv[4] );
		}
	} else if ( !strcmp(argv[2], "printtags") ) {
		ret = printtags(argv[3]);
    } else {
        print_usage(argv[0]);
    }
    loki_closeproduct(product);
    return ret;
}
