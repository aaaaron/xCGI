// xCGI
// Routines for the Common Gateway Interface used with http.
//
// Aaron Abelard / aaron@abelard.com 
//
// date__ who_____ what
// 960927 aaron    start
// 961017 aaron    first beta
// 961024 aaron    added xCGI_delete
// 961027 aaron    added xCGI_raw_dumpfile
// 961101 aaron    added xCGI_dumpfile
// 961203 aaron    corrected differences between GET and POST methods in xCGI_parse()
//                 added __xCGI_debug
// 961209 aaron    made xCGI_html_error support variable parameters

#include	<stdio.h>
#include	<stdarg.h>
#include	"xcgi.h"
#include	<sys/types.h>
#include	<string.h>
#include	<strings.h>

//define __DEBUG__

#define DEBUG_FILE "/usr13/WWW/Public/webdev/xCGI/debug.log"

#define ENVIRONMENT_VARIABLES "AUTH_TYPE CONTENT_LENGTH GATEWAY_INTERFACE HTTP_ACCEPT HTTP_USER_AGENT \
PATH_INFO PATH_TRANSLATED QUERY_STRING REMOTE_ADDR REMOTE_HOST \
REMOTE_IDENT REMOTE_USER REQUEST_METHOD SCRIPT_NAME SERVER_NAME \
SERVER_PORT SERVER_PROTOCOL SERVER_SOFTWARE HTTP_REFERER HTTP_COOKIE"

void	_xCGI_getword(char *word, char *line, char stop);      // essentially strtok which modifies strings
char	_xCGI_x2c(char *);
void	_xCGI_unescape_url(char *);
void	_xCGI_plustospace(char *);
void	_xCGI_dumpenv(void);

#ifdef __DEBUG__
void  __xCGI_debug(char *, ...);                               // for debugging purposes
#endif


// Generates a standardized error.
// error number is any integer number, format and following variables follow limited printf convention
void xCGI_html_error(int error_number, char *error_format, ...) {

	va_list       ap;
	char        * p;

	va_start(ap, error_format);

	printf("<HTML>\n");
	printf("<HEAD>\n");
	printf("<TITLE>Error</TITLE>\n");
	printf("</HEAD>\n");
	printf("<BODY>\n");
	printf("<H1>\nError %d\n</H1>\n", error_number);
	printf("\n");
	for(p = error_format; *p; p++) {
		if(*p != '%') {
			fputc(*p, stdout);
			continue;
		}
		switch(*++p) {
			case 'd':
				printf("%d", va_arg(ap, int));
				break;
			case 'f':
				printf("%f", va_arg(ap, double));
				break;
			case 's':
				printf("%s", va_arg(ap, char *));
				break;
			default:
				fputc(*p, stdout);
		}
	}
	va_end(ap);
	printf("\n");
	printf("<P><HR><ADDRESS></ADDRESS>\n");
	printf("</BODY>\n");
	printf("</HTML>\n");

	exit(error_number);

} // html_error


// Returns a character pointer to the value of a field with name passed in.
char *xCGI_find(xcgi *data, char *find) {

	xcgi	*current;

	current = data;
	while(current != NULL) {
		if(!strcasecmp(current->name, find)) {
			return(current->valu);
		}
		current = current->next;
	}
	return(NULL);

} // xCGI_find


// Deletes a node from the linked list
// Function deletes node based on its name, and is case insensitive
int xCGI_delete(xcgi **datap, char *find) {

	xcgi	*current; 
	xcgi	*last;

	last = NULL;
	current = *datap;
	while(current != NULL) {
		if(!strcasecmp(current->name, find)) {
			if(last == NULL) {
				*datap = current->next;
			} else {
				if(current->next == NULL) {
					last->next = NULL;
				} else {
					last->next = current->next;
					current=current->next;
				}
			}	 
			return(1);
		}
		last = current;
		current = current->next;
	}
	return(0);

} // xCGI_delete


// Retrieves data passed to CGI (via POST, GET, or both)
// Builds linked list based on data.
xcgi *xCGI_parse() {

	int	chars_read=0;
	int	debug_spinner=0;
	int	Icontent_length;

	char 	*content_length;
	char 	*query_string;
	char 	*request_method;
	char	*temp;
	char 	*from_stdin;
	char  *new_string;

	xcgi 	*current;
	xcgi 	*data;

	request_method = (char *) getenv("REQUEST_METHOD");
	if(request_method == NULL) {
		xCGI_html_error(430, "No Request Method");
		return(NULL);
	} 
	if(strcasecmp(request_method, "GET") && strcasecmp(request_method, "POST")) {
		xCGI_html_error(435, "Invalid Request Method");	
	}
	query_string = (char *) getenv("QUERY_STRING");

	if(!strcasecmp(request_method, "POST")) {                                            // Adds contents of stdin to QUERY_STRING
		content_length = (char *) getenv("CONTENT_LENGTH");
		if(content_length == NULL) {
			xCGI_html_error(440, "Server Error");
		}
		Icontent_length = atoi(content_length);			
		Icontent_length++;                                                           // 961217 this is needed b/c fgets reads int - 1 from stream!
		from_stdin = (char *) malloc((Icontent_length + 3) * sizeof(char));
		fgets(from_stdin, Icontent_length, stdin);
		if(from_stdin == NULL) {
			xCGI_html_error(441, "POST but no input");
		}
#ifdef __DEBUG__
		printf("from standard in: %s\n", from_stdin);
#endif
		if(query_string != NULL) {                                                   // added 961206, broken for POST only queries?
			new_string = (char *) malloc(sizeof(char) * strlen(query_string) + strlen(from_stdin) + 3);
			strcpy(new_string, query_string);
			query_string = new_string;
			strcat(query_string, "&");                                           // add & for parsing purposes
			strcat(query_string, from_stdin);
			strcat(query_string, "\0");                                          // append NULL terminator
		} else {
			query_string = (char *) malloc(sizeof(char) * strlen(from_stdin));
			strcat(query_string, from_stdin);
		}
	}

	if(query_string == NULL) {
		xCGI_html_error(440, "No data passed to cgi.");
	}
#ifdef __DEBUG__
	printf("Query String: [)]%s[(] <br>\n", query_string);
#endif
	temp = (char *) strtok(query_string, "&");
	data = current = (xcgi *) malloc(sizeof(xcgi));
	current->next = NULL;
	while(temp != NULL) {
		strcpy(current->valu, temp);
		temp = (char *) strtok(NULL, "&");
		if(temp == NULL) {
			current->next = NULL;
		} else {
			current->next = (xcgi *) malloc(sizeof(xcgi));
			current = current-> next;
			current->next = NULL;
		}
	}
	free(temp);
	current = data;
	while(current != NULL) {
		_xCGI_getword(current->name, current->valu, '=');
		_xCGI_plustospace(current->name);
		_xCGI_plustospace(current->valu);
		_xCGI_unescape_url(current->name);
		_xCGI_unescape_url(current->valu);
		current = current->next;
	}

	return (data);

} // parse


// Moves all characters from line up to stop into word
// Removes all characters from line up to and including stop from line
void _xCGI_getword(char *word, char *line, char stop) {

	int x, y = 0;

	for(x = 0; ((line[x] && (line[x] != stop))); x++) {
		word[x] = line[x];
	}

	word[x] = '\0';                                 // terminates word with a null
	if(line[x]) ++x;                                // increments x by one to skip stop character

	while(line[y++] = line[x++]);                   // Moves everything in line left x places.

} // getword


// Converts hexadecimal into character symbol.
char _xCGI_x2c(char *what) {

	register char digit;

	digit = (what[0] >= 'A' ? ((what[0] & 0xdf) - 'A')+10 : (what[0] - '0'));
	digit *= 16;
	digit += (what[1] >= 'A' ? ((what[1] & 0xdf) - 'A')+10 : (what[1] - '0'));

	return(digit);
}


// Converts standard %xx format to correct character
void _xCGI_unescape_url(char *url) {

	register int x, y;

	for(x=0, y=0; url[y]; ++x,++y) {
		if((url[x] = url[y]) == '%') {
			url[x] = _xCGI_x2c(&url[y+1]);
			y+=2;
		}
	}
	url[x] = '\0';
}


// Converts all '+' in string to ' '
void _xCGI_plustospace(char *str) {

	register int x;

	for(x=0; str[x]; x++) if(str[x] == '+') str[x] = ' ';
}


// Prints file to standard output
void xCGI_raw_dumpfile(char *filename) {

	FILE *filepointer;
	char errordesc[150];
	char temp[255];

	filepointer = fopen(filename, "r");
	if(filepointer == NULL) {
		strcpy(errordesc, "Can't open file: ");
		strcpy(errordesc, filename);
		xCGI_html_error(505, errordesc);
	}
	while(!feof(filepointer)) {
		fgets(temp, 255, filepointer);
		printf("%s", temp);
	}
}


// Displays current linked list data
void _xCGI_dumpdata (xcgi *data) {

	xcgi * current;

	current = data;
	while(current != NULL) {
		printf("Pntr: [)]%p[(]\n", current);
		printf("Name: [)]%s[(]\n", current->name);
		printf("Valu: [)]%s[(]\n", current->valu);
		current = current->next;
	}

}

//	dumps out all environment variables listed in ENVIRONMENT_VARIABLES
void _xCGI_dumpenv(void) {

	char *variables, *variable, *environment;

	register int i;

	//  is there a way to allocate memory and copy in one statement?
	variables = (char *) malloc(sizeof(char) * strlen(ENVIRONMENT_VARIABLES));
	strcpy(variables, ENVIRONMENT_VARIABLES);
	variable = (char *) strtok(variables, " ");
	while(variable != NULL) {
		environment = (char *) getenv(variable);
		if(environment == NULL) {
			printf("[%s] = [NULL] <br>\n", variable); 
		} else {
			printf("[%s] = [%s] <br>\n", variable, environment); 
		}
		variable = (char *) strtok(NULL, " ");
	}	
}

//looks for <xcgi value="X" alt="text string"> in file filename
// expects param_num of character strings to replace <xcgi> tags with.
/* int xCGI_dumpfile(char *filename, int param_num, ...) {

   FILE *	fp;
   va_list		parameters;

   } */


// Strips any HTML from line.
char *xCGI_strip(char *inbound_line) {

	int		inbound_len;
	int		x;
	int		z = 0;
	int		in_tag = 0;
	char	*	outbound_line;

	inbound_len = strlen(inbound_line);
	outbound_line = (char *) malloc(sizeof(char) * inbound_len);

	for(x = 0; x < inbound_len; x++) {
		if(inbound_line[x] == '<') {
			in_tag = 1;
		}
		if(!in_tag) {
			outbound_line[z++] = inbound_line[x];
		}
		if(inbound_line[x] == '>') {
			in_tag = 0;
		}
	}

	return(outbound_line);

}


// Logs debuging messages to DEBUG_FILE
// Supports limited printf syntax.
#ifdef __DEBUG__
void __xCGI_debug(char *fmt, ...) {

	int           x;
	va_list       ap;
	FILE        * fp;
	char        * param;
	char        * p;

	va_start(ap, fmt);
	fp = fopen(DEBUG_FILE, "a");
	if(fp == NULL) {
	} else {
		for(p = fmt; *p; p++) {
			if(*p != '%') {
				fputc(*p, fp);
				continue;
			}
			switch(*++p) {
				case 'd':
					fprintf(fp, "%d", va_arg(ap, int));
					break;
				case 'f':
					fprintf(fp, "%f", va_arg(ap, double));
					break;
				case 's':
					fprintf(fp, "%s", va_arg(ap, char *));
					break;
				default:
					fputc(*p, fp);
					break;
			}
		}
	}
	va_end(ap);
	fclose(fp);
}
#endif


