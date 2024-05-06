/*	see copyright notice in squirrel.h */
#ifndef _SQSTDIO_H_
#define _SQSTDIO_H_

#define SQSTD_STREAM_TYPE_TAG 0x80000000

struct SQStream {
	virtual ~SQStream() {}
	virtual SQInteger Read(void *buffer, SQInteger size) = 0;
	virtual SQInteger Write(void *buffer, SQInteger size) = 0;
	virtual SQInteger Flush() = 0;
	virtual SQInteger Tell() = 0;
	virtual SQInteger Len() = 0;
	virtual SQInteger Seek(SQInteger offset, SQInteger origin) = 0;
	virtual bool IsValid() = 0;
	virtual bool EOS() = 0;
};

#define SQ_SEEK_CUR 0
#define SQ_SEEK_END 1
#define SQ_SEEK_SET 2

typedef void* SQFILE;

SQUIRREL_API SQFILE sqstd_fopen(const SQChar *,const SQChar *);
SQUIRREL_API SQInteger sqstd_fread(SQUserPointer, SQInteger, SQInteger, SQFILE);
SQUIRREL_API SQInteger sqstd_fwrite(const SQUserPointer, SQInteger, SQInteger, SQFILE);
SQUIRREL_API SQInteger sqstd_fseek(SQFILE , SQInteger , SQInteger);
SQUIRREL_API SQInteger sqstd_ftell(SQFILE);
SQUIRREL_API SQInteger sqstd_fflush(SQFILE);
SQUIRREL_API SQInteger sqstd_fclose(SQFILE);
SQUIRREL_API SQInteger sqstd_feof(SQFILE);

template <Squirk T>
SQUIRREL_API SQRESULT sqstd_createfile(HSQUIRRELVM<T> v, SQFILE file,SQBool own);
template <Squirk T>
SQUIRREL_API SQRESULT sqstd_getfile(HSQUIRRELVM<T> v, SQInteger idx, SQFILE *file);

//compiler helpers
template <Squirk T>
SQUIRREL_API SQRESULT sqstd_loadfile(HSQUIRRELVM<T> v,const SQChar *filename,SQBool printerror);
template <Squirk T>
SQUIRREL_API SQRESULT sqstd_dofile(HSQUIRRELVM<T> v,const SQChar *filename,SQBool retval,SQBool printerror);
template <Squirk T>
SQUIRREL_API SQRESULT sqstd_writeclosuretofile(HSQUIRRELVM<T> v,const SQChar *filename);

template <Squirk T>
SQUIRREL_API SQRESULT sqstd_register_iolib(HSQUIRRELVM<T> v);

#endif /*_SQSTDIO_H_*/

