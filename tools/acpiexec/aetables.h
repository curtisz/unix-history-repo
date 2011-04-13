/******************************************************************************
 *
 * Module Name: aetables.h - Precompiled AML ACPI tables for acpiexec
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2011, Intel Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#ifndef __AETABLES_H__
#define __AETABLES_H__


/*
 * Miscellaneous pre-compiled AML ACPI tables to be installed
 */

/* Default DSDT. This will be replaced with the input DSDT */

static unsigned char DsdtCode[] =
{
    0x44,0x53,0x44,0x54,0x24,0x00,0x00,0x00,  /* 00000000    "DSDT$..." */
    0x02,0x6F,0x49,0x6E,0x74,0x65,0x6C,0x00,  /* 00000008    ".oIntel." */
    0x4E,0x75,0x6C,0x6C,0x44,0x53,0x44,0x54,  /* 00000010    "NullDSDT" */
    0x01,0x00,0x00,0x00,0x49,0x4E,0x54,0x4C,  /* 00000018    "....INTL" */
    0x04,0x12,0x08,0x20,
};

static unsigned char LocalDsdtCode[] =
{
    0x44,0x53,0x44,0x54,0x24,0x00,0x00,0x00,  /* 00000000    "DSDT$..." */
    0x02,0x2C,0x49,0x6E,0x74,0x65,0x6C,0x00,  /* 00000008    ".,Intel." */
    0x4C,0x6F,0x63,0x61,0x6C,0x00,0x00,0x00,  /* 00000010    "Local..." */
    0x01,0x00,0x00,0x00,0x49,0x4E,0x54,0x4C,  /* 00000018    "....INTL" */
    0x30,0x07,0x09,0x20,
};

/* Several example SSDTs */

static unsigned char Ssdt1Code[] = /* Has method _T98 */
{
    0x53,0x53,0x44,0x54,0x30,0x00,0x00,0x00,  /* 00000000    "SSDT0..." */
    0x01,0xB8,0x49,0x6E,0x74,0x65,0x6C,0x00,  /* 00000008    "..Intel." */
    0x4D,0x61,0x6E,0x79,0x00,0x00,0x00,0x00,  /* 00000010    "Many...." */
    0x01,0x00,0x00,0x00,0x49,0x4E,0x54,0x4C,  /* 00000018    "....INTL" */
    0x24,0x04,0x03,0x20,0x14,0x0B,0x5F,0x54,  /* 00000020    "$.. .._T" */
    0x39,0x38,0x00,0x70,0x0A,0x04,0x60,0xA4,  /* 00000028    "98.p..`." */
};

static unsigned char Ssdt2Code[] = /* Has method _T99 */
{
    0x53,0x53,0x44,0x54,0x30,0x00,0x00,0x00,  /* 00000000    "SSDT0..." */
    0x01,0xB7,0x49,0x6E,0x74,0x65,0x6C,0x00,  /* 00000008    "..Intel." */
    0x4D,0x61,0x6E,0x79,0x00,0x00,0x00,0x00,  /* 00000010    "Many...." */
    0x01,0x00,0x00,0x00,0x49,0x4E,0x54,0x4C,  /* 00000018    "....INTL" */
    0x24,0x04,0x03,0x20,0x14,0x0B,0x5F,0x54,  /* 00000020    "$.. .._T" */
    0x39,0x39,0x00,0x70,0x0A,0x04,0x60,0xA4,  /* 00000028    "99.p..`." */
};

unsigned char Ssdt3Code[] =     /* Has method _T97 */
{
    0x54,0x53,0x44,0x54,0x30,0x00,0x00,0x00,  /* 00000000    "TSDT0..." */
    0x01,0xB8,0x49,0x6E,0x74,0x65,0x6C,0x00,  /* 00000008    "..Intel." */
    0x4D,0x61,0x6E,0x79,0x00,0x00,0x00,0x00,  /* 00000010    "Many...." */
    0x01,0x00,0x00,0x00,0x49,0x4E,0x54,0x4C,  /* 00000018    "....INTL" */
    0x24,0x04,0x03,0x20,0x14,0x0B,0x5F,0x54,  /* 00000020    "$.. .._T" */
    0x39,0x37,0x00,0x70,0x0A,0x04,0x60,0xA4,  /* 00000028    "97.p..`." */
};

/* Example OEM table */

static unsigned char Oem1Code[] =
{
    0x4F,0x45,0x4D,0x31,0x38,0x00,0x00,0x00,  /* 00000000    "OEM18..." */
    0x01,0x4B,0x49,0x6E,0x74,0x65,0x6C,0x00,  /* 00000008    ".KIntel." */
    0x4D,0x61,0x6E,0x79,0x00,0x00,0x00,0x00,  /* 00000010    "Many...." */
    0x01,0x00,0x00,0x00,0x49,0x4E,0x54,0x4C,  /* 00000018    "....INTL" */
    0x18,0x09,0x03,0x20,0x08,0x5F,0x58,0x54,  /* 00000020    "... ._XT" */
    0x32,0x0A,0x04,0x14,0x0C,0x5F,0x58,0x54,  /* 00000028    "2...._XT" */
    0x31,0x00,0x70,0x01,0x5F,0x58,0x54,0x32,  /* 00000030    "1.p._XT2" */
};

/* ASL source for this table is at the end of this file */

static unsigned char OemxCode[] =
{
    0x4F,0x45,0x4D,0x58,0xB0,0x00,0x00,0x00,  /* 00000000    "OEMX...." */
    0x02,0x54,0x4D,0x79,0x4F,0x45,0x4D,0x00,  /* 00000008    ".TMyOEM." */
    0x54,0x65,0x73,0x74,0x00,0x00,0x00,0x00,  /* 00000010    "Test...." */
    0x32,0x04,0x00,0x00,0x49,0x4E,0x54,0x4C,  /* 00000018    "2...INTL" */
    0x31,0x03,0x10,0x20,0x14,0x1D,0x5F,0x49,  /* 00000020    "1.. .._I" */
    0x4E,0x49,0x00,0x70,0x0D,0x54,0x61,0x62,  /* 00000028    "NI.p.Tab" */
    0x6C,0x65,0x20,0x4F,0x45,0x4D,0x58,0x20,  /* 00000030    "le OEMX " */
    0x72,0x75,0x6E,0x6E,0x69,0x6E,0x67,0x00,  /* 00000038    "running." */
    0x5B,0x31,0x10,0x22,0x5C,0x5F,0x47,0x50,  /* 00000040    "[1."\_GP" */
    0x45,0x14,0x06,0x5F,0x45,0x30,0x37,0x00,  /* 00000048    "E.._E07." */
    0x14,0x06,0x5F,0x45,0x32,0x32,0x00,0x14,  /* 00000050    ".._E22.." */
    0x06,0x5F,0x4C,0x33,0x31,0x00,0x14,0x06,  /* 00000058    "._L31..." */
    0x5F,0x4C,0x36,0x36,0x00,0x5B,0x82,0x10,  /* 00000060    "_L66.[.." */
    0x4F,0x45,0x4D,0x31,0x08,0x5F,0x50,0x52,  /* 00000068    "OEM1._PR" */
    0x57,0x12,0x05,0x02,0x0A,0x07,0x00,0x5B,  /* 00000070    "W......[" */
    0x82,0x10,0x4F,0x45,0x4D,0x32,0x08,0x5F,  /* 00000078    "..OEM2._" */
    0x50,0x52,0x57,0x12,0x05,0x02,0x0A,0x66,  /* 00000080    "PRW....f" */
    0x00,0x10,0x26,0x5C,0x47,0x50,0x45,0x32,  /* 00000088    "..&\GPE2" */
    0x14,0x06,0x5F,0x4C,0x30,0x31,0x00,0x14,  /* 00000090    ".._L01.." */
    0x06,0x5F,0x45,0x30,0x37,0x00,0x08,0x5F,  /* 00000098    "._E07.._" */
    0x50,0x52,0x57,0x12,0x0C,0x02,0x12,0x08,  /* 000000A0    "PRW....." */
    0x02,0x5C,0x47,0x50,0x45,0x32,0x01,0x00   /* 000000A8    ".\GPE2.." */
};

/* Example ECDT */

unsigned char EcdtCode[] =
{
    0x45,0x43,0x44,0x54,0x4E,0x00,0x00,0x00,  /* 00000000    "ECDTN..." */
    0x01,0x94,0x20,0x49,0x6E,0x74,0x65,0x6C,  /* 00000008    ".. Intel" */
    0x54,0x65,0x6D,0x70,0x6C,0x61,0x74,0x65,  /* 00000010    "Template" */
    0x01,0x00,0x00,0x00,0x49,0x4E,0x54,0x4C,  /* 00000018    "....INTL" */
    0x16,0x03,0x11,0x20,0x01,0x08,0x00,0x00,  /* 00000020    "... ...." */
    0x66,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  /* 00000028    "f......." */
    0x01,0x08,0x00,0x00,0x62,0x00,0x00,0x00,  /* 00000030    "....b..." */
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  /* 00000038    "........" */
    0x09,0x5C,0x5F,0x53,0x42,0x2E,0x50,0x43,  /* 00000040    ".\_SB.PC" */
    0x49,0x30,0x2E,0x45,0x43,0x00             /* 00000048    "I0.EC."   */
};


/*
 * Example installable control method
 *
 * DefinitionBlock ("", "DSDT", 2, "Intel", "MTHDTEST", 0x20090512)
 * {
 *     Method (\_SI_._T97, 1, Serialized)
 *     {
 *         Store ("Example installed method", Debug)
 *         Store (Arg0, Debug)
 *         Return ()
 *     }
 * }
 *
 * Compiled byte code below.
 */
static unsigned char MethodCode[] =
{
    0x44,0x53,0x44,0x54,0x53,0x00,0x00,0x00,  /* 00000000    "DSDTS..." */
    0x02,0xF9,0x49,0x6E,0x74,0x65,0x6C,0x00,  /* 00000008    "..Intel." */
    0x4D,0x54,0x48,0x44,0x54,0x45,0x53,0x54,  /* 00000010    "MTHDTEST" */
    0x12,0x05,0x09,0x20,0x49,0x4E,0x54,0x4C,  /* 00000018    "... INTL" */
    0x22,0x04,0x09,0x20,0x14,0x2E,0x2E,0x5F,  /* 00000020    "".. ..._" */
    0x54,0x49,0x5F,0x5F,0x54,0x39,0x37,0x09,  /* 00000028    "SI__T97." */
    0x70,0x0D,0x45,0x78,0x61,0x6D,0x70,0x6C,  /* 00000030    "p.Exampl" */
    0x65,0x20,0x69,0x6E,0x73,0x74,0x61,0x6C,  /* 00000038    "e instal" */
    0x6C,0x65,0x64,0x20,0x6D,0x65,0x74,0x68,  /* 00000040    "led meth" */
    0x6F,0x64,0x00,0x5B,0x31,0x70,0x68,0x5B,  /* 00000048    "od.[1ph[" */
    0x31,0xA4,0x00,
};


#if 0
/******************************************************************************
 *
 * DESCRIPTION: ASL tables that are used in RSDT/XSDT, also used to test
 *              Load/LoadTable operators.
 *
 *****************************************************************************/

DefinitionBlock ("", "OEMX", 2, "MyOEM", "Test", 0x00000432)
{
    External (GPE2, DeviceObj)

    Method (_INI)
    {
        Store ("Table OEMX running", Debug)
    }

    Scope (\_GPE)
    {
        Method (_E07) {}
        Method (_E22) {}
        Method (_L31) {}
        Method (_L66) {}
    }

    Device (OEM1)
    {
        Name (_PRW, Package(){7,0})
    }
    Device (OEM2)
    {
        Name (_PRW, Package(){0x66,0})
    }

    Scope (\GPE2)
    {
        Method (_L01) {}
        Method (_E07) {}

        Name (_PRW, Package() {Package() {\GPE2, 1}, 0})
    }
}

/* Parent gr.asl file */

DefinitionBlock ("", "DSDT", 2, "Intel", "Many", 0x00000001)
{
    Name (BUF1, Buffer()
    {
        0x4F,0x45,0x4D,0x58,0xB0,0x00,0x00,0x00,  /* 00000000    "OEMX...." */
        0x02,0x54,0x4D,0x79,0x4F,0x45,0x4D,0x00,  /* 00000008    ".TMyOEM." */
        0x54,0x65,0x73,0x74,0x00,0x00,0x00,0x00,  /* 00000010    "Test...." */
        0x32,0x04,0x00,0x00,0x49,0x4E,0x54,0x4C,  /* 00000018    "2...INTL" */
        0x31,0x03,0x10,0x20,0x14,0x1D,0x5F,0x49,  /* 00000020    "1.. .._I" */
        0x4E,0x49,0x00,0x70,0x0D,0x54,0x61,0x62,  /* 00000028    "NI.p.Tab" */
        0x6C,0x65,0x20,0x4F,0x45,0x4D,0x58,0x20,  /* 00000030    "le OEMX " */
        0x72,0x75,0x6E,0x6E,0x69,0x6E,0x67,0x00,  /* 00000038    "running." */
        0x5B,0x31,0x10,0x22,0x5C,0x5F,0x47,0x50,  /* 00000040    "[1."\_GP" */
        0x45,0x14,0x06,0x5F,0x45,0x30,0x37,0x00,  /* 00000048    "E.._E07." */
        0x14,0x06,0x5F,0x45,0x32,0x32,0x00,0x14,  /* 00000050    ".._E22.." */
        0x06,0x5F,0x4C,0x33,0x31,0x00,0x14,0x06,  /* 00000058    "._L31..." */
        0x5F,0x4C,0x36,0x36,0x00,0x5B,0x82,0x10,  /* 00000060    "_L66.[.." */
        0x4F,0x45,0x4D,0x31,0x08,0x5F,0x50,0x52,  /* 00000068    "OEM1._PR" */
        0x57,0x12,0x05,0x02,0x0A,0x07,0x00,0x5B,  /* 00000070    "W......[" */
        0x82,0x10,0x4F,0x45,0x4D,0x32,0x08,0x5F,  /* 00000078    "..OEM2._" */
        0x50,0x52,0x57,0x12,0x05,0x02,0x0A,0x66,  /* 00000080    "PRW....f" */
        0x00,0x10,0x26,0x5C,0x47,0x50,0x45,0x32,  /* 00000088    "..&\GPE2" */
        0x14,0x06,0x5F,0x4C,0x30,0x31,0x00,0x14,  /* 00000090    ".._L01.." */
        0x06,0x5F,0x45,0x30,0x37,0x00,0x08,0x5F,  /* 00000098    "._E07.._" */
        0x50,0x52,0x57,0x12,0x0C,0x02,0x12,0x08,  /* 000000A0    "PRW....." */
        0x02,0x5C,0x47,0x50,0x45,0x32,0x01,0x00   /* 000000A8    ".\GPE2.." */
    })

    Name (HNDL, 0)
    Method (LD)
    {
        Load (BUF1, HNDL)
        Store ("Load operator, handle:", Debug)
        Store (HNDL, Debug)
    }

    Method (MAIN, 0, NotSerialized)
    {
        Store ("Loading OEMX table", Debug)
        Store (LoadTable ("OEMX", "MyOEM", "Test"), Debug)
    }

    Scope (\_GPE)
    {
        Method (_L08) {}
        Method (_E08) {}
        Method (_L0B) {}
    }

    Device (DEV0)
    {
        Name (_PRW, Package() {0x11, 0})
    }

    Device (\GPE2)
    {
        Method (_L00) {}
    }
}

/* Example ECDT */

[000h 0000   4]                    Signature : "ECDT"    /* Embedded Controller Boot Resources Table */
[004h 0004   4]                 Table Length : 0000004E
[008h 0008   1]                     Revision : 01
[009h 0009   1]                     Checksum : 14
[00Ah 0010   6]                       Oem ID : " Intel"
[010h 0016   8]                 Oem Table ID : "Template"
[018h 0024   4]                 Oem Revision : 00000001
[01Ch 0028   4]              Asl Compiler ID : "INTL"
[020h 0032   4]        Asl Compiler Revision : 20110316


[024h 0036  12]      Command/Status Register : <Generic Address Structure>
[024h 0036   1]                     Space ID : 01 (SystemIO)
[025h 0037   1]                    Bit Width : 08
[026h 0038   1]                   Bit Offset : 00
[027h 0039   1]         Encoded Access Width : 00 (Undefined/Legacy)
[028h 0040   8]                      Address : 0000000000000066

[030h 0048  12]                Data Register : <Generic Address Structure>
[030h 0048   1]                     Space ID : 01 (SystemIO)
[031h 0049   1]                    Bit Width : 08
[032h 0050   1]                   Bit Offset : 00
[033h 0051   1]         Encoded Access Width : 00 (Undefined/Legacy)
[034h 0052   8]                      Address : 0000000000000062

[03Ch 0060   4]                          UID : 00000000
[040h 0064   1]                   GPE Number : 09
[041h 0065  13]                     Namepath : "\_SB.PCI0.EC"

#endif

#endif /* __AETABLES_H__ */
