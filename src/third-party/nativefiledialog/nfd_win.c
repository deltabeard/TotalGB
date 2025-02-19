/*
  Native File Dialog
  http://www.frogtoss.com/labs
 */

#define NFD_MAX_STRLEN 256
#define _NFD_UNUSED(x) ((void)x)
#define NFD_UTF8_BOM "\xEF\xBB\xBF"

// NFD_COMMON
// Disable warning using strncat()
#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#    define _CRT_SECURE_NO_WARNINGS
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "nfd.h"

#define FTG_IMPLEMENT_CORE
#include "ftg_core.h"


static char g_errorstr[NFD_MAX_STRLEN] = {0};

/* internal routines */
void* NFDi_Malloc(size_t bytes);
void NFDi_SetError(const char* msg);
int NFDi_SafeStrncpy(char* dst, const char* src, size_t maxCopy);
int32_t NFDi_UTF8_Strlen(const nfdchar_t* str);
int NFDi_IsFilterSegmentChar(char ch);
void NFDi_SplitPath(const char* path, const char** out_dir, const char** out_filename);

/* public routines */

const char*
NFD_GetError(void)
{
    return g_errorstr;
}

size_t
NFD_PathSet_GetCount(const nfdpathset_t* pathset)
{
    assert(pathset);
    return pathset->count;
}

nfdchar_t*
NFD_PathSet_GetPath(const nfdpathset_t* pathset, size_t num)
{
    assert(pathset);
    assert(num < pathset->count);

    return pathset->buf + pathset->indices[num];
}

void
NFD_PathSet_Free(nfdpathset_t* pathset)
{
    assert(pathset);
    free(pathset->indices);
    free(pathset->buf);
}

void
*NFDi_Calloc( size_t num, size_t size )
{
    void *ptr = calloc(num, size);
    if ( !ptr )
        NFDi_SetError("NFDi_Calloc failed.");

    return ptr;
}

/* internal routines */

void*
NFDi_Malloc(size_t bytes)
{
    void* ptr = malloc(bytes);
    if (!ptr)
        NFDi_SetError("NFDi_Malloc failed.");

    return ptr;
}

void
NFDi_SetError(const char* msg)
{
    int bTruncate = NFDi_SafeStrncpy(g_errorstr, msg, NFD_MAX_STRLEN);
    assert(!bTruncate);
    _NFD_UNUSED(bTruncate);
}


int
NFDi_SafeStrncpy(char* dst, const char* src, size_t maxCopy)
{
    size_t n = maxCopy;
    char*  d = dst;

    assert(src);
    assert(dst);

    while (n > 0 && *src != '\0') {
        *d++ = *src++;
        --n;
    }

    /* Truncation case -
       terminate string and return true */
    if (n == 0) {
        dst[maxCopy - 1] = '\0';
        return 1;
    }

    /* No truncation.  Append a single NULL and return. */
    *d = '\0';
    return 0;
}


/* adapted from microutf8 */
int32_t
NFDi_UTF8_Strlen(const nfdchar_t* str)
{
    /* This function doesn't properly check validity of UTF-8 character
    sequence, it is supposed to use only with valid UTF-8 strings. */

    int32_t       character_count = 0;
    int32_t       i = 0; /* Counter used to iterate over string. */
    nfdchar_t     maybe_bom[4];
    unsigned char c;

    /* If there is UTF-8 BOM ignore it. */
    if (strlen(str) > 2) {
        strncpy(maybe_bom, str, 3);
        maybe_bom[3] = 0;
        if (strcmp(maybe_bom, (nfdchar_t*)NFD_UTF8_BOM) == 0)
            i += 3;
    }

    while (str[i]) {
        c = (unsigned char)str[i];
        if (c >> 7 == 0) {
            /* If bit pattern begins with 0 we have ascii character. */
            ++character_count;
        } else if (c >> 6 == 3) {
            /* If bit pattern begins with 11 it is beginning of UTF-8 byte sequence. */
            ++character_count;
        } else if (c >> 6 == 2)
            ; /* If bit pattern begins with 10 it is middle of utf-8 byte sequence. */
        else {
            /* In any other case this is not valid UTF-8. */
            return -1;
        }
        ++i;
    }

    return character_count;
}

int
NFDi_IsFilterSegmentChar(char ch)
{
    return (ch == ',' || ch == ';' || ch == '\0');
}

void
NFDi_SplitPath(const char* path, const char** out_dir, const char** out_filename)
{
    // test filesystem to early out test on 'c:\path', which can't be
    // determined to not be a filename called `path` at `c:\`
    // otherwise.
    if (ftg_is_dir(path)) {
        *out_dir = path;
        *out_filename = NULL;
        return;
    }

    const char* filename = ftg_get_filename_from_path(path);
    if (filename[0]) {
        *out_filename = filename;
    }
    *out_dir = path;
}
// NFD_COMMON_END

// untested atm
// https://github.com/mlabbe/nativefiledialog/pull/65

#include <stdlib.h>

/* only locally define UNICODE in this compilation unit */
#ifndef UNICODE
#define UNICODE
#endif

#include <wchar.h>
#include <stdio.h>
#include <assert.h>
#include <windows.h>
#include <ShlObj.h>

enum {
    LEGACY_OPEN  = 0,
    LEGACY_SAVE  = 1,
    LEGACY_MULTI = 2
};

// returns the length of a wchar_t containing multiple strings
static size_t wchar_len_multi(const wchar_t *str) {
    const wchar_t *s = str;
    while(*s)
        s += wcslen(s) + 1; // advance to next substring
    return (s - str) + 1; // account for additional terminator at end of list
}

// converts wchar_t to nfdchar_t (utf8), returning a pointer to the created nfdchar_t (don't forget to free it)
static nfdchar_t *wchar_to_nfd(const wchar_t *src, const size_t src_ct) {
    size_t sz, written;
    nfdchar_t *dst = NULL;
    sz = WideCharToMultiByte(CP_UTF8, 0,
                             src, src_ct,
                             NULL, 0,
                             NULL, NULL);
    assert(sz);
    sz++;
    dst = (nfdchar_t*)NFDi_Malloc(sz);
    if(!dst)
        return NULL;
    written = WideCharToMultiByte(CP_UTF8, 0,
                                  src, src_ct,
                                  dst, sz,
                                  NULL, NULL);
    assert(written);

    return dst;
}

// converts nfdchar_t to wchar_t, returning a pointer to the created wchar_t (don't forget to free it)
// this function is also useful for converting NFD's UTF8 output for use with _wfopen()
// it is importantant that `len` is defined separately, because this works on strings containing multiple substrings and therefore multiple terminators
static wchar_t *nfd_to_wchar(const nfdchar_t *str, const size_t len) {
    if(!len)
        return NULL;
    size_t wnum, written;
    wchar_t *dst;
    wnum = MultiByteToWideChar(CP_UTF8, 0,
                               str, len+1/*+1 terminator*/,
                               NULL, 0);
    assert(wnum);
    wnum++;
    dst = (wchar_t*) NFDi_Malloc( wnum * sizeof(wchar_t) );
    if(!dst)
        return NULL;
    written = MultiByteToWideChar(CP_UTF8, 0,
                                  str, len+1/*+1 terminator*/,
                                  dst, wnum);
    assert(written);

    return dst;
}

// generates a filter and returns its length
// don't forget to free the result
static inline int filter_compose(char **dst, const nfdchar_t *src) {
    if(src==NULL)
        return 0;
    const nfdchar_t *t, *end;
    const size_t len = strlen(src) + 1;
    char *p, *buf, *b;
    end = src + len; // +1 to include terminator, which we treat the same as ';' when parsing
    *dst = (char*) NFDi_Calloc ( 1, len * 4 ); // "png,jpg;txt\0" (11+1) -> "*.png;*.jpg\0*.png;*.jpg\0*.txt\0*.txt\0" (32+4)
    if(!*dst)
        return 0;
    buf = (char*) NFDi_Calloc ( 1, len * 3 ); // extra buffer to simplify naming
    if(!buf) {
        free(*dst);
        *dst = NULL;
        return 0;
    }
    p = *dst;
    memcpy(buf, "*.", 2); // precedes first filter
    b = buf + 2;
    for(t=src; t<end; t++) {
        switch(*t) {
            case ',': // add a separate type to the current filter
                memcpy(b, ";*.", 3);
                b += 3;
                continue;
            case ';': case '\0': // push current filter contents
                *b++ = 0; // terminator
                strcpy(p, buf); // names
                p += b - buf; // advance to end
                strcpy(p, buf); // types
                p += b - buf; // advance to end
                *p = 0; // terminator
                b = buf + 2; // begin a new filter
                continue;
        }
        *b++ = *t;
    }
    free(buf);

    return p - *dst;
}

// master function to reduce redundant code
static nfdresult_t legacy_function( const nfdchar_t *filterList,
                                    const nfdchar_t *defaultPath,
                                    nfdchar_t **outPath,
                                    const int mode )
{
    nfdresult_t result = NFD_ERROR;

    OPENFILENAME ofn;

    wchar_t wpath[2048];
    wchar_t *wdefault = NULL, *wfilter = NULL;

    *outPath = NULL;

    // generate filter
    if(filterList && filterList[0]) {
        char *filter=NULL;
        int filter_len;
        filter_len = filter_compose(&filter, filterList);
        if(!filter) {
            NFDi_SetError("filter_compose() memory allocation error");
            goto cleanup;
        }
        wfilter = nfd_to_wchar(filter, filter_len);
        free(filter);
        if(!wfilter) {
            NFDi_SetError("nfd_to_wchar(filter) memory allocation error");
            goto cleanup;
        }
    }
    if(defaultPath!=NULL && defaultPath[0]) {
        wdefault = nfd_to_wchar(defaultPath, strlen(defaultPath));
        if(!wdefault) {
            NFDi_SetError("nfd_to_wchar(defaultPath) allocation error");
            goto cleanup;
        }
    }

    // initialize ofn structure
    ZeroMemory(&ofn, sizeof ofn);
    ofn.lStructSize = sizeof ofn;
    ofn.hwndOwner = NULL  ;
    ofn.lpstrFile = wpath ;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof wpath / sizeof wpath[0];
    ofn.lpstrFilter = wfilter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = wdefault;
    ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST ;
    if(mode==LEGACY_MULTI)
        ofn.Flags |= OFN_ALLOWMULTISELECT;
    if(mode==LEGACY_SAVE)
        ofn.Flags |= OFN_OVERWRITEPROMPT;

    // prompts and error checking
    if( ( mode!=LEGACY_SAVE && GetOpenFileName(&ofn) ) || ( mode==LEGACY_SAVE && GetSaveFileName(&ofn) )) { // returns non-zero on file load
        *outPath = wchar_to_nfd(ofn.lpstrFile, wchar_len_multi(ofn.lpstrFile)); // convert to utf8
        if(!*outPath) {
            NFDi_SetError("wchar_to_nfd(ofn.lpstrFile) allocation error");
            goto cleanup;
        }
        result = NFD_OKAY;
    }
    else // returns zero on cancel
        result = NFD_CANCEL;

cleanup: // cleanup
    if(wfilter)
        free(wfilter);
    if(wdefault)
        free(wdefault);

    return result;
}

/* public */

nfdresult_t NFD_OpenDialog( const nfdchar_t *filterList,
                            const nfdchar_t *defaultPath,
                            nfdchar_t **outPath )
{
    return legacy_function(filterList, defaultPath, outPath, LEGACY_OPEN);
}

nfdresult_t NFD_OpenDialogMultiple( const nfdchar_t *filterList,
                                    const nfdchar_t *defaultPath,
                                    nfdpathset_t *outPaths )
{
    nfdchar_t *t, *p, *k, *dir;
    size_t sz = 0, dir_sz, i;
    nfdresult_t result = legacy_function(filterList, defaultPath, &t, LEGACY_MULTI);
    if(result != NFD_OKAY)
        goto cleanup;
    dir = t;
    outPaths->count = 0;
    dir_sz = strlen(dir);
    p = t + dir_sz + 1;
    while(*p) {
        sz += dir_sz + strlen(p) + 1 + 1; // +1 to account for the separators we're adding, +1 to account for each path's terminator
        outPaths->count++;
        p += strlen(p) + 1; // proceed to next
    }
    if(!outPaths->count) { // only one file provided
        outPaths->buf = t;
        outPaths->indices = (size_t*) NFDi_Calloc( 1, sizeof(size_t) ); // calloc guarantees indices[0] = 0
        outPaths->count = 1;
        return result;
    }
    outPaths->buf = (nfdchar_t*) NFDi_Calloc( sz, sizeof(nfdchar_t) );
    outPaths->indices = (size_t*) NFDi_Malloc( outPaths->count * sizeof(size_t) );
    for(i=0, k=outPaths->buf, p=t; i<outPaths->count; i++) {
        p += strlen(p) + 1; // proceed to next
        if(!*p) // two '\0's in a row, end of list
            break;
        outPaths->indices[i] = k - outPaths->buf;
        memcpy(k, dir, dir_sz);
        k += dir_sz;
        *k++ = '\\'; // win32 path separator
        sz = strlen(p);
        memcpy(k, p, sz);
        k += sz;
        *k++ = '\0'; // terminator
    }

cleanup:
    if(t)
        free(t);
    return result;
}

nfdresult_t NFD_SaveDialog( const nfdchar_t *filterList,
                            const nfdchar_t *defaultPath,
                            nfdchar_t **outPath )
{
    return legacy_function(filterList, defaultPath, outPath, LEGACY_SAVE);
}

nfdresult_t NFD_PickFolder( const nfdchar_t *defaultPath,
                            nfdchar_t **outPath )
{
    _NFD_UNUSED(defaultPath); // TODO `defaultPath` is currently unused; what's a reliable way to put it to good use in legacy Win32?
    BROWSEINFO binfo;
    LPITEMIDLIST lpi;
    wchar_t wbuf[2048];

    // initialize binfo structure
    ZeroMemory(&binfo, sizeof binfo);
    binfo.pszDisplayName = wbuf;
    binfo.iImage = -1;
    binfo.ulFlags |= BIF_USENEWUI | BIF_VALIDATE | BIF_EDITBOX;

    // prompt
    lpi = SHBrowseForFolder(&binfo);

    // error testing
    if(!lpi)
        return NFD_CANCEL;
    if(!SHGetPathFromIDList(lpi, wbuf)) {
        NFDi_SetError("SHGetPathFromIDList error");
        return NFD_ERROR;
    }

    // convert to utf8
    *outPath = wchar_to_nfd(wbuf, wcslen(wbuf)+1);
    if(!*outPath) {
        NFDi_SetError("wchar_to_nfd(wbuf) allocation error");
        return NFD_ERROR;
    }

    return NFD_OKAY;
}
