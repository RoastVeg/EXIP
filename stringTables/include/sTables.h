/*==================================================================================*\
|                                                                                    |
|                    EXIP - Efficient XML Interchange Processor                      |
|                                                                                    |
|------------------------------------------------------------------------------------|
| Copyright (c) 2010, EISLAB - Luleå University of Technology                        |
| All rights reserved.                                                               |
|                                                                                    |
| Redistribution and use in source and binary forms, with or without                 |
| modification, are permitted provided that the following conditions are met:        |
|     * Redistributions of source code must retain the above copyright               |
|       notice, this list of conditions and the following disclaimer.                |
|     * Redistributions in binary form must reproduce the above copyright            |
|       notice, this list of conditions and the following disclaimer in the          |
|       documentation and/or other materials provided with the distribution.         |
|     * Neither the name of the EISLAB - Luleå University of Technology nor the      |
|       names of its contributors may be used to endorse or promote products         |
|       derived from this software without specific prior written permission.        |
|                                                                                    |
| THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND    |
| ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED      |
| WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE             |
| DISCLAIMED. IN NO EVENT SHALL EISLAB - LULEÅ UNIVERSITY OF TECHNOLOGY BE LIABLE    |
| FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES |
| (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;       |
| LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND        |
| ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT         |
| (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS      |
| SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                       |
|                                                                                    |
|                                                                                    |
|                                                                                    |
\===================================================================================*/

/**
 * @file sTables.h
 * @brief Functions describing EXI sting tables operations
 * @date Sep 14, 2010
 * @author Rumen Kyusakov
 * @version 0.1
 * @par[Revision] $Id$
 */

#ifndef STABLES_H_
#define STABLES_H_

#include "procTypes.h"
#include "errorHandle.h"

#define DEFAULT_VALUE_ROWS_NUMBER      10
#define DEFAULT_URI_ROWS_NUMBER        10
#define DEFAULT_PREFIX_ROWS_NUMBER     10
#define DEFAULT_LOCALNAMES_ROWS_NUMBER 10

/**
 * @brief Creates fresh empty ValueTable (value partition of EXI string table)
 * This operation includes allocation of memory for DEFAULT_VALUE_ROWS_NUMBER number of value rows
 * @param[out] vTable ValueTable string table
 * @return Error handling code
 */
errorCode createValueTable(ValueTable** vTable);

/**
 * @brief Creates fresh empty URITable (uri partition of EXI string table)
 * This operation includes allocation of memory for DEFAULT_URI_ROWS_NUMBER number of uri rows
 * @param[out] uTable URITable string table
 * @return Error handling code
 */
errorCode createURITable(URITable** uTable);

/**
 * @brief Creates fresh empty PrefixTable (prefix partition of EXI string table)
 * This operation includes allocation of memory for DEFAULT_PREFIX_ROWS_NUMBER number of prefix rows
 * @param[out] pTable PrefixTable string table
 * @return Error handling code
 */
errorCode createPrefixTable(PrefixTable** pTable);

/**
 * @brief Creates fresh empty LocalNamesTable (local names partition of EXI string table)
 * This operation includes allocation of memory for DEFAULT_LOCALNAMES_ROWS_NUMBER number of local names rows
 * @param[out] lTable LocalNamesTable string table
 * @return Error handling code
 */
errorCode createLocalNamesTable(LocalNamesTable** lTable);


/**
 * @brief Add new row into the URI string table
 *
 * @param[in, out] uTable URI string table
 * @param[in] uri the string representing this uri. The StringType can be allocated on the stack.
 * @param[out] rowID the ID of the row inserted
 * @return Error handling code
 */
errorCode addURIRow(URITable* uTable, StringType uri, unsigned int* rowID);

/**
 * @brief Add new row into the Local-Names string table
 *
 * @param[out] lTable Local-Names string table
 * @param[in] local_name the string representing this Local-Names. The StringType can be allocated on the stack.
 * @param[out] rowID the ID of the row inserted
 * @return Error handling code
 */
errorCode addLNRow(LocalNamesTable* lTable, StringType local_name, unsigned int* rowID);

/**
 * @brief Create string tables for an EXI stream.
 * It also inserts the default entries in the table.
 * Because the behavior depends on the EXI options of the stream
 * it is important that the options are initialized before
 * calling this function.
 *
 * @param[in, out] strm EXI stream of bits
 * @return Error handling code
 */
errorCode createInitialStringTables(EXIStream* strm);

#endif /* STABLES_H_ */
