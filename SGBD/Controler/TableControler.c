#include <stdlib.h>

#include "view.h"
#include "controler.h"

#include "../Utilities/StringVector.h"

#include "../Model/Data.h"
#include "../Model/Tuple.h"
#include "../Model/Column.h"
#include "../Model/Table.h"
#include "../Model/Database.h"

#include "TableControler.h"

static int isNumeric(char *param)
{
	if (param == NULL)
		return 0;

	for (;*param && *param >= '0' && *param <= '9'; param++);

	return *param == '\0';
}

static void toUpperCase(char* phrase)
{
	for(;*phrase;phrase++)
		*phrase = toUpper(*phrase);
}

static void disp_error_cast(Data *arg1, char *arg2, DisplayFunc fct)
{
	char error[1024];

	if (arg1->type == STR)
		ssprintf(error, "%s%s to %s", CANNOT_CAST, arg1->value.str, arg2);
	else
		ssprintf(error, "%s%d to %s", CANNOT_CAST, arg1->value.integer, arg2);

	fct(error);
}

static void disp_classic_error(char *header, char *arg, DisplayFunc fct)
{
	char* error;
	if (header == NULL || arg == NULL || fct == NULL)
		return;

	error = (char*) malloc( ( sizeof(char) * strlen(header) + ( sizeof(char) * strlen(arg) + 1 ) ) );

	if (error == NULL)
		return;

	ssprintf(error, "%s%s", header, arg);

	fct(error);

	free(error);
}

void addColInTable(Database *db, StringVector *request, DisplayFunc fct)
{
	Table *t;
	Column *c;

	if (db == NULL || request == NULL || fct == NULL)
		return;

	if (request->length < 5)
	{
		fct(NOT_ENOUGH_WORDS);
	}
	else if (request->length > 5)
	{
		fct(TOO_MUCH_WORDS);
	}

	if (! (t = getTableByName(db, request->tab[2])) )
	{
		disp_classic_error(UNKNOWN_TABLE, request->tab[2], fct);
		return;
	}

	if (getColumn(t, request->tab[3]))
	{
		disp_classic_error(EXISTING_COL, request->tab[3], fct);
		return;
	}

	toUpperCase(request->tab[4]);

	if ( strcmp(request->tab[4], TYPE_INT) && strcmp(request->tab[4], TYPE_STRING) )
	{
		disp_classic_error(UNKNOWN_TYPE, request->tab[4]);
		return;
	}

	c = createColumn(request->tab[3], ! strcmp(request->tab[4], TYPE_INT) ? INT : STR);

	addColumn(t, c);
}

void addTupleInTable(Database *db, StringVector *request, DisplayFunc fct)
{
	Table *t;
	Tuple *tuple;
	int i;

	if (db == NULL || request == NULL || fct == NULL)
		return;

	if (request->length < 4)
	{
		fct(NOT_ENOUGH_WORDS);
	}

	if (! (t = getTableByName(db, request->tab[2])) )
	{
		disp_classic_error(UNKNOWN_TABLE, request->tab[2], fct);
		return;
	}

	tuple = createTuple();

	if (t->nbColumn > request->length - 3)
		fct(NOT_ENOUGH_PARAM);
	else if (t->nbColumn < request->length - 3)
		fct(TOO_MUCH_PARAM);

	for (i = 3; i < request->length; i++)
	{
		Data data;
		DataValue value;
		DataType dT;

		if (isNumeric(request->tab[i]))
			dT = INT;
		else
			dT = STR;

		dT == INT ? value.integer = atoi(request->tab[i]) : value.str = request->tab[i];

		data = createData(dT, value);

		addData(tuple, data);
	}

	i = addTuple(t, tuple);

	if (i < 0)
	{
		fct("ERR: Unexpected error");
	}
	else if (i > 0)
	{
		DataType dT = tuple->datas[i]->type;

		if (dT == INT)
			disp_error_cast(tuple->datas[i], "INT", fct);
		else
			disp_error_cast(tuple->datas[i], "STR", fct);
	}
}

void delColFromTable(Database *db, StringVector *request, DisplayFunc fct)
{
	Table *t;

	if (db == NULL || request == NULL || fct == NULL)
		return;

	if (request->length < 4)
		fct(NOT_ENOUGH_WORDS);
	else if (request->length > 4)
		fct(TOO_MUCH_WORDS);

	if (! (t = getTableByName(db, request->tab[2])) )
	{
		disp_classic_error(UNKNOWN_TABLE, request->tab[2], fct);
		return;
	}

	if (! removeColumn(t, request->tab[3]) )
		disp_classic_error(UNKNOWN_COL, request->tab[3], fct);
}

void dispColsFromTable(Database *db, StringVector *request, DisplayFunc fct)
{
	Table *t;
	int i, length = 0;
	char *tabCol = NULL;

	if (db == NULL || request == NULL || fct == NULL)
		return;

	if (request->length < 3)
		fct(NOT_ENOUGH_WORDS);
	else if (request->length > 3)
		fct(TOO_MUCH_WORDS);

	if (! (t = getTableByName(db, request->tab[2])) )
	{
		disp_classic_error(UNKNOWN_TABLE, request->tab[2], fct);
		return;
	}

	for (i = 0; i < t->nbColumn; i++)
	{
		char *tmpPtr;

		tmpPtr = (char*) realloc(tabCol, (sizeof(char) * (strlen(t->columns[i]->name) + 7)));

		if (tmpPtr == NULL)
		{
			fct("ERR: Unexpected error");
			free(tabCol);
			return;
		}

		tabCol = tmpPtr;

		length += strlen(t->columns[i]->name) + ((i + 1) == t->nbColumn ? 4 : 5);

		strcat(tabCol, t->columns[i]->name);
		strcat(tabCol, "(");
		strcat(tabCol, t->columns[i]->type == INT ? "INT" : "STR");
		strcat(tabCol, ")");

		if (i + 1 != t->nbColumn)
			strcat(tabCol, " ");
	}
	fct(tabCol);
	free(tabCol);
}
void dispTuplesFromTable(Database *db, StringVector *request, DisplayFunc fct)
{
	Table *t;
	int i, j, length;
	char* tuple;

	if (db == NULL || request == NULL || fct == NULL)
		return;

	if (request->length < 4)
		fct(NOT_ENOUGH_WORDS);
	else if (request->length > 4)
		fct(TOO_MUCH_WORDS);

	if (! (t = getTableByName(db, request->tab[2])) )
	{
		disp_classic_error(UNKNOWN_TABLE, request->tab[2], fct);
		return;
	}

	for(i = 0; i < t->nbTuple; i++)
	{
		tuple = NULL;
		length = 0;
		for (j = 0; j < t->nbColumn; j++)
		{
			char buffer[1000], *tmp;

			Data *d = t->tuples[i]->datas[j];

			if (d->type == INT)
				sprintf(buffer, "%d",d->value.integer);
			else
				sprintf(buffer, "%s", d->value.str);

			length += strlen(buffer);

			tmp = realloc(tuple, sizeof(char) * (length + (j + 1 == t->nbColumn ? 1 : 2) ));

			if (tmp == NULL)
			{
				free(tuple);
				return;
			}

			tuple = tmp;

			strcat(tuple, buffer);

			if ((j + 1) != t->nbColumn)
				strcat(tuple, " ");
		}
		fct(tuple);
		free(tuple);
	}
}

