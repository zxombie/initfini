/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2018 Andrew Turner
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract FA8750-10-C-0237
 * ("CTSRD"), as part of the DARPA CRASH research programme.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#include <stdio.h>
#include "initfini.h"

extern void *__dso_handle;

#if defined(__aarch64__) || defined(__arm__)
#define	INIT_CALL_SEQ(func)	"bl " __STRING(func)
#elif defined(__amd64__) || defined(__i386__)
#define	INIT_CALL_SEQ(func)	"call " __STRING(func)
#elif defined(__mips__)
#define	INIT_CALL_SEQ(func)						\
    ".set noreorder		\n"					\
    "bal	1f		\n"					\
    "nop			\n"					\
    ".set reorder		\n"					\
    "1f:			\n"					\
    ".cpsetup $ra, $v0, 1b	\n"					\
    "jal	" __STRING(func)
#elif defined(__powerpc64__)
#define	INIT_CALL_SEQ(func)	"bl " __STRING(func) "; nop"
#else
#warning No .init call sequence
#endif

#define	INIT_TEST(section)						\
static void section ## _test(void) __used;				\
int section ## _pre;							\
int section ## _run;							\
int section ## _post;							\
static void								\
section ## _test(void)							\
{									\
	section ## _run = 1;						\
}									\
asm (									\
    ".section ."__STRING(section)",\"aw\"	\n"			\
    POINTER_EXPR" "__STRING(section)"_test	\n"			\
    ".text					\n")

#define	INIT_ARRAY_TEST(section)					\
extern void (*__ ## section ## _start[])(int, char **, char **);	\
extern void (*__ ## section ## _end[])(int, char **, char **);		\
INIT_TEST(section)

#define	FINI_TEST(section)						\
static void section ## _test(void) __used;				\
int section ## _pre;							\
int section ## _run;							\
int section ## _post;							\
static void								\
section ## _test(void)							\
{									\
	section ## _run = 1;						\
	printf("In ."__STRING(section)"\n");				\
	printf("pre test has %srun\n", section ## _pre ? "" : "not ");	\
	printf("post test has %srun\n", section ## _post ? "" : "not "); \
	printf("\n");							\
}									\
asm (									\
    ".section ."__STRING(section)",\"aw\"	\n"			\
    POINTER_EXPR" "__STRING(section)"_test	\n"			\
    ".text					\n")

#define	FINI_ARRAY_TEST(section)					\
extern void (*__ ## section ## _start[])(int, char **, char **);	\
extern void (*__ ## section ## _end[])(int, char **, char **);		\
FINI_TEST(section)

#define	PRINT_INIT_RESULT(section)					\
do {									\
	printf("."__STRING(section)" %srun\n",				\
	    section ## _run ? "" : "not ");				\
	printf("\n");							\
} while (0)

#define	PRINT_INITFINI_ARRAY_RESULT(section)				\
do {									\
	printf("__"__STRING(section)"_start = %p\n",			\
	    __ ## section ##_start);					\
	printf("__"__STRING(section)"_end = %p\n", __ ## section ## _end); \
	printf("."__STRING(section)" size = %ld\n",			\
	    (long)(__ ## section ## _end - __ ## section ## _start));	\
} while (0)

#define	PRINT_INIT_ARRAY_RESULT(section)				\
do {									\
	PRINT_INITFINI_ARRAY_RESULT(section);				\
	PRINT_INIT_RESULT(section);					\
} while (0)

#define	PRINT_FINI_ARRAY_RESULT(section)				\
	PRINT_INITFINI_ARRAY_RESULT(section)

#ifndef NO_INIT_ARRAY
INIT_ARRAY_TEST(preinit_array);
INIT_ARRAY_TEST(init_array);
FINI_ARRAY_TEST(fini_array);
#endif
INIT_TEST(ctors);
FINI_TEST(dtors);

#if defined(INIT_CALL_SEQ)
static void init_test(void) __used;
static void fini_test(void) __used;
int init_run;

static void
init_test(void)
{

	init_run = 1;
}
asm (
    ".section .init		\n"
    INIT_CALL_SEQ(init_test)"	\n"
    ".text			\n");

static void
fini_test(void)
{

	printf("In .fini\n");
}
asm (
    ".section .fini		\n"
    INIT_CALL_SEQ(fini_test)"	\n"
    ".text			\n");
#endif

static void constructor_test(void) __used;
int constructor_run, constructor_state;

__attribute__((constructor)) static void
constructor_test(void)
{

	constructor_run = 1;
	constructor_state = 0;
#define RECORD_STATE(section, shift)					\
do {									\
	if (section ## _pre)						\
		constructor_state |= (1 << shift);			\
	if (section ## _run)						\
		constructor_state |= (2 << shift);			\
	if (section ## _post)						\
		constructor_state |= (4 << shift);			\
} while (0)

#ifndef NO_INIT_ARRAY
	RECORD_STATE(preinit_array, 0);
	RECORD_STATE(init_array, 4);
#endif
	RECORD_STATE(ctors, 8);
}

static void destructor_test(void) __used;

__attribute__((destructor)) static void
destructor_test(void)
{

	constructor_state = 0;
#ifndef NO_INIT_ARRAY
	RECORD_STATE(fini_array, 0);
#endif
	RECORD_STATE(dtors, 0);
	printf("destructor state: %x\n", constructor_state);
	printf("\n");
}

typedef void (*func)(void);
void _Jv_RegisterClasses(const func *);

int jcr_run;
void
_Jv_RegisterClasses(const func *jcr)
{

	jcr_run = 1;
}
asm (
    ".section .jcr,\"aw\"			\n"
    POINTER_EXPR" 1				\n"
    ".text					\n");

int __cxa_atexit(void (*)(void *), void *, void *);

static void
cxa_atexit(void *ptr)
{

	printf("cxa_atexit: %p\n", ptr);
	printf("\n");
}

void
print_results(void)
{

	__cxa_atexit(cxa_atexit, NULL, __dso_handle);

#ifndef NO_INIT_ARRAY
	PRINT_INIT_ARRAY_RESULT(preinit_array);
	PRINT_INIT_ARRAY_RESULT(init_array);
#endif
	PRINT_INIT_RESULT(ctors);

#if defined(INIT_CALL_SEQ)
	printf(".init %srun\n", init_run ? "" : "not ");
#else
	printf("No INIT_CALL_SEQ defined\n");
#endif
	printf("\n");

	printf("constructor %srun\n", constructor_run ? "" : "not ");
	printf("constructor state: %x\n", constructor_state);
	printf("\n");

	printf("jcr %srun\n", jcr_run ? "" : "not ");
	printf("\n");

#ifndef NO_INIT_ARRAY
	PRINT_FINI_ARRAY_RESULT(fini_array);
	printf("\n");
#endif

	printf("__dso_handle = %p\n", __dso_handle);
	printf("\n");
}

#ifndef SHLIB
int
main(int argc, char *argv[])
{

	print_results();
	return (0);
}
#endif
