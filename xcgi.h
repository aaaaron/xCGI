
// xCGI
// Routines for the Common Gateway Interface used with http. 
//	
// Aaron Abelard / aaron@abelard.com
//
// date__ who__ ver____ what
// 960927 aja   0.10b   start
// 961018 aja   1.00    first version

// Header File

#define MAX_NAME_SIZE 1024
#define MAX_VALU_SIZE 4096

typedef struct XCGI {
        char name[MAX_NAME_SIZE];               // Name of argument
        char valu[MAX_VALU_SIZE];               // Value of argument
        struct XCGI *next;                      // Pointer to next argument
} xcgi;

//  parse
//	Takes environment variables and uses this information to create
//	a linked list of type xcgi which contain all the arguments passed
//	from the server using either the get or post method.  This function
//	will read from standard in as well as the environment.
//
//	Takes: Nothing
//
//	Returns: Pointer to head of linked list of type xcgi.
//
xcgi *xCGI_parse();

//  html_error
// 	Prints a standardized error format to the user.
//
//	Takes: int error_number, error number to report/log
//	       char *error_desc, string which explains error
//
//	Returns: void
//	
void xCGI_html_error(int error_number, char *error_format, ...);

//	find
//	Finds particular record in CGI input
//
//  Takes: Pointer to head of xcgi list
//	       Pointer to character string which is name of variable to find
//
//  Returns: Pointer to character string which is value of variable found or NULL if not found.
//
char *xCGI_find(xcgi *, char *);

//	raw_dumpfile
//  dumps file to stdout
void xCGI_raw_dumpfile(char *);

// delete
//		removes pair named with char *.  note, must pass pointer to head.
int  xCGI_delete(xcgi **, char *);

// strip
//		strips any html from a string.
char *xCGI_strip(char *);

// dumpfile
//    uncompleted...
int   xCGI_dumpfile(char *, ...);

