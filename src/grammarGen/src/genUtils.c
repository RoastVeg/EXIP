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
 * @file genUtils.c
 * @brief Implementation of utility functions for generating Schema-informed Grammar definitions
 * @date Nov 23, 2010
 * @author Rumen Kyusakov
 * @version 0.1
 * @par[Revision] $Id$
 */

#include "genUtils.h"
#include "memManagement.h"
#include "grammarRules.h"
#include "eventsEXI.h"
#include "stringManipulate.h"
#include "sortingAlgorithms.h"

struct globalElSortingStruct
{
	GrammarQnameArray* glQnameArr;
	URITable* uTable;
};

static int sortingURITable(unsigned int prev_val, unsigned int cur_val, void* args);
static int sortingLNTable(unsigned int prev_val, unsigned int cur_val, void* args);
static int sortingGlobalElems(unsigned int prev_val, unsigned int cur_val, void* args);

errorCode concatenateGrammars(AllocList* memList, struct EXIGrammar* left, struct EXIGrammar* right, struct EXIGrammar** result)
{
	int i = 0;
	int j = 0;
	*result = (struct EXIGrammar*) memManagedAllocate(memList, sizeof(struct EXIGrammar));
	if(*result == NULL)
		return MEMORY_ALLOCATION_ERROR;

	(*result)->nextInStack = NULL;
	(*result)->rulesDimension = left->rulesDimension + right->rulesDimension;
	(*result)->ruleArray = (GrammarRule*) memManagedAllocate(memList, sizeof(GrammarRule)*((*result)->rulesDimension));
	if((*result)->ruleArray == NULL)
		return MEMORY_ALLOCATION_ERROR;

	/* The Non-terminal IDs must be unique within the particular grammar.
	 * To ensure this is true after the concatenating, the right Non-terminal IDs
	 * are re-numerated starting from the biggest left Non-terminal ID value + 1*/

	for(;i < left->rulesDimension; i++)
	{
		copyGrammarRule(memList, &(left->ruleArray[i]), &((*result)->ruleArray[i]), 0);
		(*result)->ruleArray[i].nonTermID = GR_SCHEMA_GRAMMARS_FIRST + i;

		j = 0;
		for(; j < (*result)->ruleArray[i].prodCount; j++)
		{
			if((*result)->ruleArray[i].prodArray[j].event.eventType == EVENT_EE)
			{
				(*result)->ruleArray[i].prodArray[j].event.eventType = EVENT_VOID;
				(*result)->ruleArray[i].prodArray[j].nonTermID = GR_SCHEMA_GRAMMARS_FIRST + left->rulesDimension;
			}
		}
	}

	for(i = 0;i < right->rulesDimension; i++)
	{
		copyGrammarRule(memList, &(right->ruleArray[i]), &((*result)->ruleArray[left->rulesDimension + i]), left->rulesDimension);
		(*result)->ruleArray[left->rulesDimension + i].nonTermID = GR_SCHEMA_GRAMMARS_FIRST + left->rulesDimension + i;
	}

	return ERR_OK;
}

errorCode createElementProtoGrammar(AllocList* memList, StringType name, StringType target_ns,
									struct EXIGrammar* typeDef, QName scope, unsigned char nillable,
									struct EXIGrammar** result)
{
	// TODO: Element-i,0 : Type-j,0 - this basically means that the Element Grammar equals to the type grammar
	// Here only needs to add already normalized type grammar in a element grammar pool
	// So remove the code below as it is not needed
	// Also name and target_ns should be added to metaStrTable if not already there

	int i = 0;

	*result = (struct EXIGrammar*) memManagedAllocate(memList, sizeof(struct EXIGrammar));
	if(*result == NULL)
		return MEMORY_ALLOCATION_ERROR;

	(*result)->nextInStack = NULL;
	(*result)->rulesDimension = typeDef->rulesDimension;
	(*result)->ruleArray = (GrammarRule*) memManagedAllocate(memList, sizeof(GrammarRule)*((*result)->rulesDimension));
	if((*result)->ruleArray == NULL)
		return MEMORY_ALLOCATION_ERROR;

	for(;i < typeDef->rulesDimension; i++)
	{
		copyGrammarRule(memList, &(typeDef->ruleArray[i]), &((*result)->ruleArray[i]), 0);
		(*result)->ruleArray[i].nonTermID = GR_SCHEMA_GRAMMARS_FIRST + i;
	}

	return ERR_OK;
}

errorCode createSimpleTypeGrammar(AllocList* memList, QName simpleType, struct EXIGrammar** result)
{
	errorCode tmp_err_code = UNEXPECTED_ERROR;
	EXIEvent event;

	*result = (struct EXIGrammar*) memManagedAllocate(memList, sizeof(struct EXIGrammar));
	if(*result == NULL)
		return MEMORY_ALLOCATION_ERROR;

	(*result)->nextInStack = NULL;
	(*result)->rulesDimension = 2;
	(*result)->ruleArray = (GrammarRule*) memManagedAllocate(memList, sizeof(GrammarRule)*((*result)->rulesDimension));
	if((*result)->ruleArray == NULL)
		return MEMORY_ALLOCATION_ERROR;

	tmp_err_code = initGrammarRule(&((*result)->ruleArray[0]), memList);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;
	(*result)->ruleArray[0].nonTermID = GR_SCHEMA_GRAMMARS_FIRST;
	event.eventType = EVENT_CH;
	tmp_err_code = getEXIDataType(simpleType, &(event.valueType));
	tmp_err_code = addProduction(&((*result)->ruleArray[0]), getEventCode1(0), event, GR_SCHEMA_GRAMMARS_FIRST + 1);

	tmp_err_code = initGrammarRule(&((*result)->ruleArray[1]), memList);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;
	(*result)->ruleArray[1].nonTermID = GR_SCHEMA_GRAMMARS_FIRST + 1;
	tmp_err_code = addProduction(&((*result)->ruleArray[1]), getEventCode1(0), getEventDefType(EVENT_EE), GR_VOID_NON_TERMINAL);

	return ERR_OK;
}

errorCode createSimpleEmptyTypeGrammar(AllocList* memList, struct EXIGrammar** result)
{
	errorCode tmp_err_code = UNEXPECTED_ERROR;
	*result = (struct EXIGrammar*) memManagedAllocate(memList, sizeof(struct EXIGrammar));
	if(*result == NULL)
		return MEMORY_ALLOCATION_ERROR;

	(*result)->nextInStack = NULL;
	(*result)->rulesDimension = 1;
	(*result)->ruleArray = (GrammarRule*) memManagedAllocate(memList, sizeof(GrammarRule)*((*result)->rulesDimension));
	if((*result)->ruleArray == NULL)
		return MEMORY_ALLOCATION_ERROR;

	tmp_err_code = initGrammarRule(&((*result)->ruleArray[0]), memList);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;
	(*result)->ruleArray[0].nonTermID = GR_SCHEMA_GRAMMARS_FIRST;
	tmp_err_code = addProduction(&((*result)->ruleArray[0]), getEventCode1(0), getEventDefType(EVENT_EE), GR_VOID_NON_TERMINAL);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;
	return ERR_OK;
}

errorCode createComplexTypeGrammar(AllocList* memList, StringType name, StringType target_ns,
		                           struct EXIGrammar* attrUsesArray, unsigned int attrUsesArraySize,
		                           StringType* wildcardArray, unsigned int wildcardArraySize,
		                           struct EXIGrammar* contentTypeGrammar,
								   struct EXIGrammar** result)
{
	//TODO: Implement the case when there are wildcards i.e. wildcardArray is not empty
	//TODO: Consider freeing the intermediate grammars which are not longer needed resulting from the use of concatenateGrammars()

	struct EXIGrammar* tmpGrammar;

	errorCode tmp_err_code = UNEXPECTED_ERROR;
	unsigned int i = 1;

	tmpGrammar = &(attrUsesArray[0]);
	for(; i < attrUsesArraySize; i++)
	{
		tmp_err_code = concatenateGrammars(memList, tmpGrammar, &(attrUsesArray[i]), &tmpGrammar);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
	}

	tmp_err_code = concatenateGrammars(memList, tmpGrammar, contentTypeGrammar, result);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;

	return ERR_OK;
}

errorCode createComplexEmptyTypeGrammar(AllocList* memList, StringType name, StringType target_ns,
		                           struct EXIGrammar* attrUsesArray, unsigned int attrUsesArraySize,
		                           StringType* wildcardArray, unsigned int wildcardArraySize,
								   struct EXIGrammar** result)
{
	//TODO: Implement the case when there are wildcards i.e. wildcardArray is not empty
	//TODO: Consider freeing the intermediate grammars which are not longer needed resulting from the use of concatenateGrammars()

	struct EXIGrammar* tmpGrammar;

	errorCode tmp_err_code = UNEXPECTED_ERROR;
	unsigned int i = 1;
	struct EXIGrammar* emptyContent;

	tmpGrammar = &(attrUsesArray[0]);
	for(; i < attrUsesArraySize; i++)
	{
		tmp_err_code = concatenateGrammars(memList, tmpGrammar, &(attrUsesArray[i]), &tmpGrammar);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
	}

	tmp_err_code = createSimpleEmptyTypeGrammar(memList, &emptyContent);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;

	tmp_err_code = concatenateGrammars(memList, tmpGrammar, emptyContent, result);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;

	return ERR_OK;
}

errorCode createComplexUrTypeGrammar(AllocList* memList, struct EXIGrammar** result)
{
	return NOT_IMPLEMENTED_YET;
}

errorCode createComplexUrEmptyTypeGrammar(AllocList* memList, struct EXIGrammar** result)
{
	return NOT_IMPLEMENTED_YET;
}

errorCode createAttributeUseGrammar(AllocList* memList, unsigned char required, StringType name, StringType target_ns,
										  QName simpleType, QName scope, struct EXIGrammar** result, URITable* metaURITable, DynArray* regProdQname)
{
	errorCode tmp_err_code = UNEXPECTED_ERROR;
	EXIEvent event1;
	uint32_t metaUriID = 0;
	struct productionQname pqRow;
	uint32_t elIndx = 0;
	uint32_t metaLnID = 0;

	*result = (struct EXIGrammar*) memManagedAllocate(memList, sizeof(struct EXIGrammar));
	if(*result == NULL)
		return MEMORY_ALLOCATION_ERROR;

	(*result)->nextInStack = NULL;
	(*result)->rulesDimension = 2;
	(*result)->ruleArray = (GrammarRule*) memManagedAllocate(memList, sizeof(GrammarRule)*((*result)->rulesDimension));
	if((*result)->ruleArray == NULL)
		return MEMORY_ALLOCATION_ERROR;

	tmp_err_code = initGrammarRule(&((*result)->ruleArray[0]), memList);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;
	(*result)->ruleArray[0].nonTermID = GR_SCHEMA_GRAMMARS_FIRST;
	event1.eventType = EVENT_AT_QNAME;
	tmp_err_code = getEXIDataType(simpleType, &event1.valueType);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;

	tmp_err_code = addProduction(&((*result)->ruleArray[0]), getEventCode1(0), event1, GR_SCHEMA_GRAMMARS_FIRST + 1);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;

	if(!lookupURI(metaURITable, target_ns, &metaUriID)) // URI not found in the meta string tables
	{
		tmp_err_code = addURIRow(metaURITable, target_ns, &metaUriID, memList);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
	}
	(*result)->ruleArray[0].prodArray[0].uriRowID = metaUriID;

	if(metaURITable->rows[metaUriID].lTable == NULL)
	{
		tmp_err_code = createLocalNamesTable(&metaURITable->rows[metaUriID].lTable, memList, 0);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
	}

	if(!lookupLN(metaURITable->rows[metaUriID].lTable, name, &metaLnID)) // Local name not found in the meta string tables
	{
		tmp_err_code = addLNRow(metaURITable->rows[metaUriID].lTable, name, &metaLnID);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
	}
	(*result)->ruleArray[0].prodArray[0].lnRowID = metaLnID;

	pqRow.p_uriRowID = &((*result)->ruleArray[0].prodArray[0].uriRowID);
	pqRow.p_lnRowID = &((*result)->ruleArray[0].prodArray[0].lnRowID);
	pqRow.uriRowID_old = metaUriID;
	pqRow.lnRowID_old = metaLnID;

	tmp_err_code = addDynElement(regProdQname, &pqRow, &elIndx, memList);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;

	if(!required)
	{
		tmp_err_code = addProduction(&((*result)->ruleArray[0]), getEventCode1(0), getEventDefType(EVENT_EE), GR_VOID_NON_TERMINAL);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
	}

	tmp_err_code = initGrammarRule(&((*result)->ruleArray[1]), memList);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;
	(*result)->ruleArray[1].nonTermID = GR_SCHEMA_GRAMMARS_FIRST + 1;

	tmp_err_code = addProduction(&((*result)->ruleArray[1]), getEventCode1(0), getEventDefType(EVENT_EE), GR_VOID_NON_TERMINAL);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;

	return ERR_OK;
}

errorCode createParticleGrammar(AllocList* memList, unsigned int minOccurs, int32_t maxOccurs,
								struct EXIGrammar* termGrammar, struct EXIGrammar** result)
{
	errorCode tmp_err_code = UNEXPECTED_ERROR;
	struct EXIGrammar* tmpGrammar;
	unsigned int i = 0;

	tmpGrammar = termGrammar;
	for(; i < minOccurs; i++)
	{
		tmp_err_code = concatenateGrammars(memList, tmpGrammar, termGrammar, &tmpGrammar);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
	}

	if(maxOccurs - minOccurs > 0 || maxOccurs < 0) // Only if maxOccurs is unbounded or maxOccurs > minOccurs
	{
		unsigned char prodEEFound = 0;
		i = 0;
		for(; i < termGrammar->ruleArray[0].prodCount; i++)
		{
			if(termGrammar->ruleArray[0].prodArray[i].nonTermID == GR_VOID_NON_TERMINAL && termGrammar->ruleArray[0].prodArray[i].event.eventType == EVENT_EE)
			{
				prodEEFound = 1;
				break;
			}
		}
		if(!prodEEFound) //	There is no production Gi,0 : EE so add one
		{
			tmp_err_code = addProduction(&(termGrammar->ruleArray[0]), getEventCode1(0), getEventDefType(EVENT_EE), GR_VOID_NON_TERMINAL);
			if(tmp_err_code != ERR_OK)
				return tmp_err_code;
		}

		if(maxOccurs >= 0) // {max occurs} is not unbounded
		{
			i = 0;
			for(; i < maxOccurs - minOccurs; i++)
			{
				tmp_err_code = concatenateGrammars(memList, tmpGrammar, termGrammar, &tmpGrammar);
				if(tmp_err_code != ERR_OK)
					return tmp_err_code;
			}
			*result = tmpGrammar;
		}
		else // {max occurs} is unbounded
		{
			int j = 0;
			i = 1;  // Excluding the first rule
			for(; i < termGrammar->rulesDimension; i++)
			{
				j = 0;
				for(; j < termGrammar->ruleArray[i].prodCount; j++)
				{
					if(termGrammar->ruleArray[i].prodArray[j].nonTermID == GR_VOID_NON_TERMINAL && termGrammar->ruleArray[i].prodArray[j].event.eventType == EVENT_EE)
					{
						termGrammar->ruleArray[i].prodArray[j].nonTermID = GR_SCHEMA_GRAMMARS_FIRST;
						termGrammar->ruleArray[i].prodArray[j].event.eventType = EVENT_VOID;
					}
				}
			}

			tmp_err_code = concatenateGrammars(memList, tmpGrammar, termGrammar, result);
			if(tmp_err_code != ERR_OK)
				return tmp_err_code;
		}
	}
	else // maxOccurs == minOccurs
	{
		*result = tmpGrammar;
	}

	return ERR_OK;
}

errorCode createElementTermGrammar(AllocList* memList, StringType name, StringType target_ns,
								   struct EXIGrammar** result, URITable* metaURITable, DynArray* regProdQname)
{
	//TODO: enable support for {substitution group affiliation} property of the elements

	errorCode tmp_err_code = UNEXPECTED_ERROR;
	uint32_t metaUriID = 0;
	uint32_t metaLnID = 0;
	struct productionQname pqRow;
	uint32_t elIndx = 0;
	*result = (struct EXIGrammar*) memManagedAllocate(memList, sizeof(struct EXIGrammar));
	if(*result == NULL)
		return MEMORY_ALLOCATION_ERROR;

	(*result)->nextInStack = NULL;
	(*result)->rulesDimension = 2;
	(*result)->ruleArray = (GrammarRule*) memManagedAllocate(memList, sizeof(GrammarRule)*((*result)->rulesDimension));
	if((*result)->ruleArray == NULL)
		return MEMORY_ALLOCATION_ERROR;

	tmp_err_code = initGrammarRule(&((*result)->ruleArray[0]), memList);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;
	(*result)->ruleArray[0].nonTermID = GR_SCHEMA_GRAMMARS_FIRST;
	tmp_err_code = addProduction(&((*result)->ruleArray[0]), getEventCode1(0), getEventDefType(EVENT_SE_QNAME), GR_SCHEMA_GRAMMARS_FIRST + 1);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;

	if(!lookupURI(metaURITable, target_ns, &metaUriID)) // URI not found in the meta string tables
	{
		tmp_err_code = addURIRow(metaURITable, target_ns, &metaUriID, memList);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
	}
	(*result)->ruleArray[0].prodArray[0].uriRowID = metaUriID;

	if(metaURITable->rows[metaUriID].lTable == NULL)
	{
		tmp_err_code = createLocalNamesTable(&metaURITable->rows[metaUriID].lTable, memList, 0);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
	}

	if(!lookupLN(metaURITable->rows[metaUriID].lTable, name, &metaLnID)) // Local name not found in the meta string tables
	{
		tmp_err_code = addLNRow(metaURITable->rows[metaUriID].lTable, name, &metaLnID);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
	}
	(*result)->ruleArray[0].prodArray[0].lnRowID = metaLnID;

	pqRow.p_uriRowID = &((*result)->ruleArray[0].prodArray[0].uriRowID);
	pqRow.p_lnRowID = &((*result)->ruleArray[0].prodArray[0].lnRowID);
	pqRow.uriRowID_old = metaUriID;
	pqRow.lnRowID_old = metaLnID;

	tmp_err_code = addDynElement(regProdQname, &pqRow, &elIndx, memList);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;

	tmp_err_code = initGrammarRule(&((*result)->ruleArray[1]), memList);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;
	(*result)->ruleArray[0].nonTermID = GR_SCHEMA_GRAMMARS_FIRST + 1;
	tmp_err_code = addProduction(&((*result)->ruleArray[0]), getEventCode1(0), getEventDefType(EVENT_EE), GR_VOID_NON_TERMINAL);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;

	return ERR_OK;
}

errorCode createWildcardTermGrammar(AllocList* memList, StringType* wildcardArray, unsigned int wildcardArraySize,
								   struct EXIGrammar** result)
{
	return NOT_IMPLEMENTED_YET;
}

errorCode createSequenceModelGroupsGrammar(AllocList* memList, struct EXIGrammar* pTermArray, unsigned int pTermArraySize,
											struct EXIGrammar** result)
{
	errorCode tmp_err_code = UNEXPECTED_ERROR;
	if(pTermArraySize == 0)
	{
		tmp_err_code = createSimpleEmptyTypeGrammar(memList, result);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
	}
	else
	{
		struct EXIGrammar* tmpGrammar = &(pTermArray[0]);
		int i = 1;
		for(; i < pTermArraySize; i++)
		{
			tmp_err_code = concatenateGrammars(memList, tmpGrammar, &(pTermArray[i]), &tmpGrammar);
			if(tmp_err_code != ERR_OK)
				return tmp_err_code;
		}
		*result = tmpGrammar;
	}
	return ERR_OK;
}

errorCode createChoiceModelGroupsGrammar(AllocList* memList, struct EXIGrammar* pTermArray, unsigned int pTermArraySize,
											struct EXIGrammar** result)
{
	errorCode tmp_err_code = UNEXPECTED_ERROR;
	*result = (struct EXIGrammar*) memManagedAllocate(memList, sizeof(struct EXIGrammar));
	if(*result == NULL)
		return MEMORY_ALLOCATION_ERROR;

	(*result)->nextInStack = NULL;
	(*result)->rulesDimension = 1;
	(*result)->ruleArray = (GrammarRule*) memManagedAllocate(memList, sizeof(GrammarRule)*((*result)->rulesDimension));
	if((*result)->ruleArray == NULL)
		return MEMORY_ALLOCATION_ERROR;

	tmp_err_code = initGrammarRule(&((*result)->ruleArray[0]), memList);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;
	(*result)->ruleArray[0].nonTermID = GR_SCHEMA_GRAMMARS_FIRST;

	if(pTermArraySize == 0)
	{
		tmp_err_code = addProduction(&((*result)->ruleArray[0]), getEventCode1(0), getEventDefType(EVENT_EE), GR_VOID_NON_TERMINAL);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
	}
	else
	{
		// TODO: check this case. Probably something is missing here. As maybe the
		//       particle term grammars should be added as a rules here. Link: http://www.w3.org/TR/2009/CR-exi-20091208/#choiceGroupTerms
	}

	return ERR_OK;
}

errorCode createAllModelGroupsGrammar(AllocList* memList, struct EXIGrammar* pTermArray, unsigned int pTermArraySize,
											struct EXIGrammar** result)
{
	return NOT_IMPLEMENTED_YET;
}

errorCode getEXIDataType(QName simpleXSDType, ValueType* exiType)
{
	if(strEqualToAscii(*simpleXSDType.localName, "string") ||
	   strEqualToAscii(*simpleXSDType.localName, "duration") ||
	   strEqualToAscii(*simpleXSDType.localName, "anyURI"))
	{
		*exiType = VALUE_TYPE_STRING;
		return ERR_OK;
	}
	else if(strEqualToAscii(*simpleXSDType.localName, "boolean"))
	{
		*exiType = VALUE_TYPE_BOOLEAN;
		return ERR_OK;
	}
	else if(strEqualToAscii(*simpleXSDType.localName, "integer") ||
			strEqualToAscii(*simpleXSDType.localName, "nonPositiveInteger") ||
			strEqualToAscii(*simpleXSDType.localName, "long") ||
			strEqualToAscii(*simpleXSDType.localName, "nonNegativeInteger") ||
			strEqualToAscii(*simpleXSDType.localName, "int") ||
			strEqualToAscii(*simpleXSDType.localName, "short") ||
			strEqualToAscii(*simpleXSDType.localName, "byte") ||
			strEqualToAscii(*simpleXSDType.localName, "negativeInteger") ||
			strEqualToAscii(*simpleXSDType.localName, "positiveInteger"))
	{
		*exiType = VALUE_TYPE_INTEGER;
		return ERR_OK;
	}
	else if(strEqualToAscii(*simpleXSDType.localName, "float") ||
				strEqualToAscii(*simpleXSDType.localName, "double"))
	{
		*exiType = VALUE_TYPE_FLOAT;
		return ERR_OK;
	}
	else if(strEqualToAscii(*simpleXSDType.localName, "decimal"))
	{
		*exiType = VALUE_TYPE_DECIMAL;
		return ERR_OK;
	}
	else if(strEqualToAscii(*simpleXSDType.localName, "hexBinary") ||
				strEqualToAscii(*simpleXSDType.localName, "base64Binary"))
	{
		*exiType = VALUE_TYPE_BINARY;
		return ERR_OK;
	}
	else if(strEqualToAscii(*simpleXSDType.localName, "dateTime") ||
			strEqualToAscii(*simpleXSDType.localName, "time") ||
			strEqualToAscii(*simpleXSDType.localName, "date") ||
			strEqualToAscii(*simpleXSDType.localName, "gYearMonth") ||
			strEqualToAscii(*simpleXSDType.localName, "gYear") ||
			strEqualToAscii(*simpleXSDType.localName, "gMonthDay") ||
			strEqualToAscii(*simpleXSDType.localName, "gDay") ||
			strEqualToAscii(*simpleXSDType.localName, "gMonth"))
	{
		*exiType = VALUE_TYPE_DATE_TIME;
		return ERR_OK;
	}

	return INCONSISTENT_PROC_STATE;
}

static int sortingURITable(unsigned int prev_val, unsigned int cur_val, void* args)
{
	URITable* metaURITable = (URITable*) args;
	return str_compare(metaURITable->rows[prev_val].string_val, metaURITable->rows[cur_val].string_val);
}

static int sortingLNTable(unsigned int prev_val, unsigned int cur_val, void* args)
{
	LocalNamesTable* lnTable = (LocalNamesTable*) args;
	return str_compare(lnTable->rows[prev_val].string_val, lnTable->rows[cur_val].string_val);
}

errorCode stringTablesSorting(AllocList* memList, URITable* metaURITable, ExipSchema* schema, DynArray* regProdQname)
{
	errorCode tmp_err_code = UNEXPECTED_ERROR;
	unsigned int* tmpSortUriArray;
	unsigned int i;
	unsigned int n;
	uint32_t dID = 0;
	DynArray* tmpLnTablePointes;

	tmpSortUriArray = EXIP_MALLOC(metaURITable->rowCount * sizeof(unsigned int));
	if(tmpSortUriArray == NULL)
		return MEMORY_ALLOCATION_ERROR;

	tmp_err_code = createDynArray(&tmpLnTablePointes, sizeof(unsigned int *), 10, memList);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;

	for(i = 0; i < metaURITable->rowCount; i++)
		tmpSortUriArray[i] = i;

//	The first four entries are not sorted
//	URI	0	"" [empty string]
//	URI	1	"http://www.w3.org/XML/1998/namespace"
//	URI	2	"http://www.w3.org/2001/XMLSchema-instance"
//	URI	3	"http://www.w3.org/2001/XMLSchema"
	insertionSort(tmpSortUriArray + 4, metaURITable->rowCount - 4, sortingURITable, metaURITable);

	tmp_err_code = createURITable(&(schema->initialStringTables), &schema->memList, metaURITable->rowCount);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;

	for(i = 0; i < metaURITable->rowCount; i++)
	{
		schema->initialStringTables->rows[tmpSortUriArray[i]].string_val.length = metaURITable->rows[i].string_val.length;
		schema->initialStringTables->rows[tmpSortUriArray[i]].string_val.str = metaURITable->rows[i].string_val.str;

		if(metaURITable->rows[i].lTable != NULL)
		{
			unsigned int initialEntries = 0;
			unsigned int lnRowCount = metaURITable->rows[i].lTable->rowCount;
			unsigned int* tmpSortLnArray = EXIP_MALLOC(lnRowCount * sizeof(unsigned int));
			if(tmpSortLnArray == NULL)
				return MEMORY_ALLOCATION_ERROR;

			for(n = 0; n < lnRowCount; n++)
				tmpSortLnArray[n] = n;

			addDynElement(tmpLnTablePointes, &tmpSortLnArray, &dID, memList);

			//	The initialEntries entries in "http://www.w3.org/XML/1998/namespace",
			//	"http://www.w3.org/2001/XMLSchema-instance" and "http://www.w3.org/2001/XMLSchema"
			//  are not sorted
			if(i == 1) // "http://www.w3.org/XML/1998/namespace"
			{
				initialEntries = 4;
			}
			else if(i == 2) // "http://www.w3.org/2001/XMLSchema-instance"
			{
				initialEntries = 2;
			}
			else if(i == 3) // "http://www.w3.org/2001/XMLSchema"
			{
				initialEntries = 46;
			}
			insertionSort(tmpSortLnArray + initialEntries, lnRowCount - initialEntries, sortingLNTable, metaURITable->rows[i].lTable);

			tmp_err_code = createLocalNamesTable(&(schema->initialStringTables->rows[tmpSortUriArray[i]].lTable), &schema->memList, lnRowCount);
			if(tmp_err_code != ERR_OK)
				return tmp_err_code;

			for(n = 0; n < lnRowCount; n++)
			{
				schema->initialStringTables->rows[tmpSortUriArray[i]].lTable->rows[tmpSortLnArray[n]].string_val.length = metaURITable->rows[i].lTable->rows[n].string_val.length;
				schema->initialStringTables->rows[tmpSortUriArray[i]].lTable->rows[tmpSortLnArray[n]].string_val.str = metaURITable->rows[i].lTable->rows[n].string_val.str;
			}
			schema->initialStringTables->rows[tmpSortUriArray[i]].lTable->rowCount = lnRowCount;
		}
		else
		{
			schema->initialStringTables->rows[tmpSortUriArray[i]].lTable = NULL;
			addDynElement(tmpLnTablePointes, &(metaURITable->rows[i].lTable), &dID, memList);
		}
		schema->initialStringTables->rows[tmpSortUriArray[i]].pTable = NULL;
	}
	schema->initialStringTables->rowCount = metaURITable->rowCount;

	{
		struct productionQname* pq = (struct productionQname*) regProdQname->elements;

		for(i = 0; i < regProdQname->elementCount; i++)
		{
			unsigned int* lnTable = ((unsigned int**) tmpLnTablePointes->elements)[i];
			*(pq[i].p_uriRowID) = tmpSortUriArray[pq[i].uriRowID_old];
			if(lnTable != NULL)
				*(pq[i].p_lnRowID) = lnTable[pq[i].uriRowID_old];
			else
				return INCONSISTENT_PROC_STATE; // There must be such local name table
 		}
	}

	for(i = 0; i < tmpLnTablePointes->elementCount; i++)
	{
		if(((unsigned int**) tmpLnTablePointes->elements)[i] != NULL)
		{
			EXIP_MFREE(((unsigned int**) tmpLnTablePointes->elements)[i]);
		}
	}

	EXIP_MFREE(tmpSortUriArray);
	return ERR_OK;
}

static int sortingGlobalElems(unsigned int prev_val, unsigned int cur_val, void* args)
{
	struct globalElSortingStruct* srtStruct = (struct globalElSortingStruct*) args;
	int uri_cmp_res = str_compare(srtStruct->uTable->rows[srtStruct->glQnameArr->elems[prev_val].uriRowId].string_val, srtStruct->uTable->rows[srtStruct->glQnameArr->elems[cur_val].uriRowId].string_val);
	if(uri_cmp_res == 0) // equal URIs
	{
		return str_compare(srtStruct->uTable->rows[srtStruct->glQnameArr->elems[prev_val].uriRowId].lTable->rows[srtStruct->glQnameArr->elems[prev_val].lnRowId].string_val, srtStruct->uTable->rows[srtStruct->glQnameArr->elems[cur_val].uriRowId].lTable->rows[srtStruct->glQnameArr->elems[cur_val].lnRowId].string_val);
	}
	return uri_cmp_res;
}

errorCode sortGlobalElements(AllocList* memList, DynArray* globalElements, ExipSchema* schema)
{
	unsigned int* tmpSortGlArray;
	struct globalElSortingStruct tmpSortingStruct;
	unsigned int i;
	struct grammarQname* glArr;

	schema->globalElems.elems = memManagedAllocate(&schema->memList, sizeof(struct grammarQname) * globalElements->elementCount);
	if(schema->globalElems.elems == NULL)
		return MEMORY_ALLOCATION_ERROR;

	schema->globalElems.count = globalElements->elementCount;
	glArr = (struct grammarQname*) globalElements->elements;

	tmpSortGlArray = EXIP_MALLOC(schema->globalElems.count * sizeof(unsigned int));
	if(tmpSortGlArray == NULL)
		return MEMORY_ALLOCATION_ERROR;

	for(i = 0; i < schema->globalElems.count; i++)
		tmpSortGlArray[i] = i;

	tmpSortingStruct.glQnameArr = &schema->globalElems;
	tmpSortingStruct.uTable = schema->initialStringTables;

	insertionSort(tmpSortGlArray, schema->globalElems.count, sortingGlobalElems, &tmpSortingStruct);

	for(i = 0; i < schema->globalElems.count; i++)
	{
		schema->globalElems.elems[tmpSortGlArray[i]].uriRowId = glArr[i].uriRowId;
		schema->globalElems.elems[tmpSortGlArray[i]].lnRowId = glArr[i].lnRowId;
	}

	EXIP_MFREE(tmpSortGlArray);
	return ERR_OK;
}
