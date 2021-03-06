/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          This work is licensed under BSD 3-Clause License          |
|  The full license terms and conditions are located in LICENSE.txt  |
\===================================================================*/

/**
 * @file createGrammars.c
 * @brief Generate EXI grammars from XML schema definition
 *
 * @date Oct 13, 2010
 * @author Rumen Kyusakov
 * @version 0.5
 * @par[Revision] $Id: decodeTestEXI.c 93 2011-03-30 15:39:41Z kjussakov $
 */

#include "procTypes.h"
#include "schemaOutputUtils.h"
#include "grammars.h"
#include "sTables.h"
#include <stdio.h>
#include <time.h>
#include "createGrammars.h"

errorCode toText(EXIPSchema* schemaPtr, FILE *outfile)
{
    EXIGrammar* tmpGrammar;
	QNameID qnameId = {0, 0};

	fprintf(outfile, "\n/** GLOBAL ELEMENT GRAMMARS **/\n\n");
	for(qnameId.uriId = 0; qnameId.uriId < schemaPtr->uriTable.count; qnameId.uriId++)
	{
		for(qnameId.lnId = 0; qnameId.lnId < schemaPtr->uriTable.uri[qnameId.uriId].lnTable.count; qnameId.lnId++)
		{
			tmpGrammar = GET_ELEM_GRAMMAR_QNAMEID(schemaPtr, qnameId);
			if(tmpGrammar != NULL)
			{
				if(EXIP_OK != recursiveTextGrammarOutput(qnameId, GET_LN_URI_QNAME(schemaPtr->uriTable, qnameId).elemGrammar, tmpGrammar, schemaPtr, outfile))
				{
					printf("\n ERROR: OUT_TEXT output format!");
					return EXIP_UNEXPECTED_ERROR;
				}
			}
		}
	}

	fprintf(outfile, "\n/** GLOBAL TYPE GRAMMARS **/\n\n");
	for(qnameId.uriId = 0; qnameId.uriId < schemaPtr->uriTable.count; qnameId.uriId++)
	{
		for(qnameId.lnId = 0; qnameId.lnId < schemaPtr->uriTable.uri[qnameId.uriId].lnTable.count; qnameId.lnId++)
		{
			tmpGrammar = GET_TYPE_GRAMMAR_QNAMEID(schemaPtr, qnameId);
			if(tmpGrammar != NULL)
			{
				if(EXIP_OK != textGrammarOutput(qnameId, GET_LN_URI_QNAME(schemaPtr->uriTable, qnameId).typeGrammar, tmpGrammar, schemaPtr, outfile))
				{
					printf("\n ERROR: OUT_TEXT output format!");
					return EXIP_UNEXPECTED_ERROR;
				}
			}
		}
	}

	return EXIP_OK;
}

errorCode toStaticSrc(EXIPSchema* schemaPtr, char* prefix, FILE *outfile, Deviations dvis)
{
	time_t now;
    Index count;
	Index uriId;
	Index stId, stIdMax;
	Index grIter;
	EXIGrammar* tmpGrammar;

	time(&now);
	fprintf(outfile, "/** AUTO-GENERATED: %.24s\n  * Copyright (c) 2010 - 2011, Rumen Kyusakov, EISLAB, LTU\n  * $Id$ */\n\n",  ctime(&now));

	fprintf(outfile, "/** Compilation parameters:\n");
	if(dvis.grammar != 0 || dvis.ln != 0 || dvis.url != 0 || dvis.pfx != 0)
	{
		fprintf(outfile, "  * Compiled for possible deviations from the schema:\n  * URLs: %d\n  * Local names: %d\n  * Prefixes: %d\n  * Built-in grammars: %d */\n\n", dvis.url, dvis.ln, dvis.pfx, dvis.grammar);
	}
	else
		fprintf(outfile, "  * Compiled for no deviations from the schema! (lower memory usage) */\n\n");


	fprintf(outfile, "#include \"procTypes.h\"\n\n");
	fprintf(outfile, "#define CONST\n\n");

	staticStringTblDefsOutput(&schemaPtr->uriTable, prefix, outfile);

	for(grIter = 0; grIter < schemaPtr->grammarTable.count; grIter++)
	{
		staticProductionsOutput(&schemaPtr->grammarTable.grammar[grIter], prefix, grIter, outfile);
		staticRulesOutput(&schemaPtr->grammarTable.grammar[grIter], prefix, grIter, outfile);
	}

	/* The array of schema-informed EXI grammars in the EXIPSchema object */
	fprintf(outfile, "static CONST EXIGrammar %sgrammarTable[%u] =\n{\n", prefix, (unsigned int) schemaPtr->grammarTable.count + dvis.grammar);
	for(grIter = 0; grIter < schemaPtr->grammarTable.count; grIter++)
	{
		tmpGrammar = &schemaPtr->grammarTable.grammar[grIter];
		fprintf(outfile,"   {%srule_%u, %u, %u},\n",
				prefix,
				(unsigned int) grIter,
				(unsigned int) tmpGrammar->props,
				(unsigned int) tmpGrammar->count);
	}
	// Reserving space for built-in grammar deviations
	for(grIter = 0; grIter < dvis.grammar; grIter++)
	{
		fprintf(outfile,"   {NULL, 0, 0},\n");
	}

	fprintf(outfile, "};\n\n");

	/* Build the Prefix and LN table structures */
	for(uriId = 0; uriId < schemaPtr->uriTable.count; uriId++)
	{
		/* Prefix table */
		staticPrefixOutput(&schemaPtr->uriTable.uri[uriId].pfxTable, prefix, uriId, dvis, outfile);
		/* Ln table */
		staticLnEntriesOutput(&schemaPtr->uriTable.uri[uriId].lnTable, prefix, uriId, dvis, outfile);
	}

	/* Build the URI table structure */
	staticUriTableOutput(&schemaPtr->uriTable, prefix, dvis, outfile);

	/* Build the document grammar */
	staticDocGrammarOutput(&schemaPtr->docGrammar, prefix, outfile);

	/* Build the simple types structure */
	fprintf(outfile, "static CONST SimpleType %ssimpleTypes[%u] =\n{\n", prefix, (unsigned int) schemaPtr->simpleTypeTable.count);

    if(schemaPtr->simpleTypeTable.sType != NULL)
	{
		stIdMax = schemaPtr->simpleTypeTable.count;
		for(stId = 0; stId < stIdMax; stId++)
		{
			fprintf(outfile,
					"    {%d, %d, 0x%016lX, 0x%016lX}%s",
					schemaPtr->simpleTypeTable.sType[stId].content,
					schemaPtr->simpleTypeTable.sType[stId].length,
					(long unsigned) schemaPtr->simpleTypeTable.sType[stId].max,
					(long unsigned) schemaPtr->simpleTypeTable.sType[stId].min,
					stId==(stIdMax-1) ? "\n};\n\n" : ",\n");
		}
	}

    /* Enum table entries */
    staticEnumTableOutput(schemaPtr, prefix, outfile);

	/* Finally, build the schema structure */
	fprintf(outfile,
            "CONST EXIPSchema %sschema =\n{\n",
            prefix);


	fprintf(outfile, "    {NULL, NULL},\n");

    count = schemaPtr->uriTable.count;
	fprintf(outfile,
            "    {{sizeof(UriEntry), %u, %u}, %suriEntry, %u},\n",
            (unsigned int) count,
            (unsigned int) count + dvis.url,
            prefix,
            (unsigned int) count);

	fprintf(outfile,
            "    {%sdocGrammarRule, %u, %u},\n",
            prefix,
            (unsigned int) schemaPtr->docGrammar.props,
            (unsigned int) schemaPtr->docGrammar.count);

    count = schemaPtr->simpleTypeTable.count;
	fprintf(outfile,
            "    {{sizeof(SimpleType), %u, %u}, %ssimpleTypes, %u},\n",
            (unsigned int) count,
            (unsigned int) count,
            prefix,
            (unsigned int) count);

    count = schemaPtr->grammarTable.count;
	fprintf(outfile,
            "    {{sizeof(EXIGrammar), %u, %u}, %sgrammarTable, %u},\n    %u,\n",
            (unsigned int) count,
            (unsigned int) count + dvis.grammar,
            prefix,
            (unsigned int) count,
            (unsigned int) count);

    count = schemaPtr->enumTable.count;
	fprintf(outfile,
            "    {{sizeof(EnumDefinition), %u, %u}, %s%s, %u}\n};\n\n",

            (unsigned int) count,
            (unsigned int) count,
            count == 0?"":prefix, count == 0?"NULL":"enumTable",
			(unsigned int) count);

	return EXIP_OK;
}

errorCode toDynSrc(EXIPSchema* schemaPtr, FILE *outfile)
{
	return EXIP_NOT_IMPLEMENTED_YET;
}

errorCode toEXIP(EXIPSchema* schemaPtr, FILE *outfile)
{
	return EXIP_NOT_IMPLEMENTED_YET;
}
