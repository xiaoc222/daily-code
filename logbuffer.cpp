#define LOG_CHARS 16
#include <ctype.h>
static void LogBuffer (const uint8_t* p, unsigned n)
{
char hex[(3*LOG_CHARS)+1];
char txt[LOG_CHARS+1];
unsigned odx = 0, idx = 0, at = 0;

for ( idx = 0; idx < n; idx++){
uint8_t byte = p[idx];
sprintf(hex + 3*odx, "%2.02X ", byte);
txt[odx++] = isprint(byte) ? byte : '.';
if ( odx == LOG_CHARS ){
txt[odx] = hex[3*odx] = '\0';
ALOGI("[%5u] %s %s\n", at, hex, txt);
at = idx + 1;
odx = 0;
}
}
if ( odx ){
txt[odx] = hex[3*odx] = '\0';
ALOGI("[%5u] %-48.48s %s\n", at, hex, txt);
}
}



