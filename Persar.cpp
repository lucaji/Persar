#include <stdio.h>
#include <stdlib.h>
#include <iostream.h>
#include <string.h>

/***************************/
/*	       Persar      */
/* written by Luca Cipressi*/
/*   all rights reserved   */ 
/*  v. 1b del 30/01/2001   */
/*  v1.1b del 01/02/2001   */
/*  v  2b del 02/02/2001   */
/*  v2.1b del 05/02/2001   */
/*  v2.2b del 08/02/2001   */
/*v2.2.1b del 19/02/2001   */
/*v2.2.2b del 20/02/2001   */
/*v2.2.3b del 21/02/2001   */
/***************************/

class record
{
	public:
		char		word[128];
		char		ch;
		unsigned	max;
		record		*next;

		record() { next=NULL; };
};
typedef record *RecordListPtr;


class level
{
	public:
		char*	word[128];
		int		wordn;

	char* GetString(int index);
	void SetString(int index, char*str, int len);
};

const unsigned kFileErr = 1;
const unsigned kFEOF = 2;
const unsigned kBadCommandLine = 255;

const unsigned kIsPrefix = 1;
const unsigned kIsWord = 2;
const unsigned kNone = 3;
const unsigned kLevelNotFound = 0;

const unsigned no_Err = 0;

const unsigned kMaxSolutions = 3;

//globals
level 		liv[128];
unsigned int 		AlphabethCharNumber;
unsigned	MaxLevelNumber;
char*		theWord;
char*		theFile="testo.txt";
FILE*		theStream;
FILE*		FINE;
bool		autoFlag=true;
char*		inBuffer;
unsigned long		masterPtr=0;
unsigned long		MaxApplLimit;
char*		solutions[4];
char*		tempSol3rd;
char*		tempSol4th;
unsigned long		masterPtrStorage=0;
unsigned long		masterPtrStorage4=0;

// prototypes
void FreeListFromMemory(RecordListPtr item);
void PrintList(RecordListPtr item);
void MidS(char *s, char* dest, int start, int len);
unsigned Correct(char* word);
bool AreEnoughSimilar(char *word1, char *word2);
bool StartsWith(char *what, char *where);
int GetTokenType(char *token, int theLevel);

RecordListPtr FindTheSameWord(RecordListPtr theSelection, char *theWord);
RecordListPtr TryMaxLikelihood(RecordListPtr theSelection, char *theWord);
RecordListPtr TryLookAhead(RecordListPtr theSelection, char *theWord);
RecordListPtr CreateSelectionFromLevel(int theLevel);

void AppendRecord(RecordListPtr &curr, char* s);
void MakeLevel(int	lev, int index, int source);
int ReadFile();
void printfile();
void printhelp();
void newcodes(char *argv[]);
void WriteFile();
void StripChar(char* wrd, unsigned n);
void InsChar(char* wrd, unsigned n, char c);

bool Parse(char* word, unsigned &k, unsigned &level);
bool CharBelongsToOurAlphabeth(char c);

short CloseDataStream(FILE* f);
short GetCharFromStream(FILE* f, char &ch);
void AppendCharToCStr(char c, char *s);
short OpenDataStream(char* nomefile, FILE* &f);

bool ItCanBeAnotherWord(char* ts);
int LoadBuffer(char* bufferPtr);

int GetCorrectionNumber(char* s1, char* s2);

// implementation
char* level::GetString(int index)
{
	if (word[index] == NULL)
	{ // inizializza a zero
		word[index] = (char*)malloc(256);
		*(word[index]) = 0;
	}
	return word[index];
}

void level::SetString(int index, char* str, int len)
{
	int i;
	
	word[index] = GetString(index);

	for (i=0; i<len; i++)
		*(word[index]+i) = *(str+i);

	*(word[index]+i) = 0;
}

void FreeListFromMemory(RecordListPtr item)
{
	if (item != NULL)
	{
		FreeListFromMemory(item->next);
		delete item;
	}
}

void PrintList(RecordListPtr item)
{
	if (item != NULL)
	{
		cout<<item->word<<"\n";
		PrintList(item->next);
	}
}


// ritorna una lista linkata di records
// corrispondenti al livello indicato da theLevel
RecordListPtr CreateSelectionFromLevel(int theLevel)
{
	RecordListPtr tempSel = NULL;
	
	for (int i=0; i<liv[theLevel].wordn; i++)
		AppendRecord(tempSel, liv[theLevel].GetString(i));
	return tempSel;
}

// ritorna il tipo di token immesso
// se appartenente al livello specificato
int GetTokenType(char *token, int theLevel)
{
	RecordListPtr	levSel;
	int			retVal;

	levSel = CreateSelectionFromLevel(theLevel);
	if (levSel == NULL)
		retVal = kLevelNotFound;
	else if (FindTheSameWord(levSel, token) != NULL)
		retVal = kIsWord;
	else if (TryLookAhead(levSel, token) != NULL)
		retVal = kIsPrefix;
	else
		retVal = kNone;

	FreeListFromMemory(levSel);
	return retVal;
}

void MidS(char *s, char* dest, int start, int len)
{
	int	j;
	
	if (dest != NULL)
	{
		start--;
		for (j=0; j<len; j++)
			*(dest+j) = *(s+j+start);
		*(dest+j) = 0;
	}
}

void StripChar(char* wrd, unsigned n)
{
	n--;
	for (unsigned i=n; i<strlen(wrd); i++)
		*(wrd+i)=*(wrd+i+1);
}

void InsChar(char* wrd, unsigned n, char c)
{
	int ln;
	
	n--;
	ln = strlen(wrd);
	*(wrd+ln+1) = 0;
	for (unsigned i=ln; i>n; i--)
		*(wrd+i)=*(wrd+i-1);
	*(wrd+n+1) = c;
}

bool CharBelongsToOurAlphabeth(char c)
{
	RecordListPtr	sel;
	char*		theS;
	bool		retVal = false;
	
	theS = (char*)malloc(2);

	*theS=0;
	AppendCharToCStr(c, theS);
	sel = CreateSelectionFromLevel(0);
	if (FindTheSameWord(sel, theS))
		retVal = true;
	
	free(theS);
	FreeListFromMemory(sel);
	return retVal;
}

int GetCorrectionNumber(char* s1, char* s2)
{
	int	l, i;
	int	count=1;
	
	l = strlen(s1);
	
	if (strcmp(s1, s2) == 0)
		return 1;
	else
		for (i=0; i<l; i++)
			if (*(s1+i) != *(s2+i))
				count++;
	if (count <= 4)
		return count;
	else
		return -1;	
}

bool Parse(char* word, unsigned &k, unsigned &level)
{
	RecordListPtr	tempRecPtr;
	RecordListPtr	basePtr;
	char*			copy;
	char*			temp;
	unsigned		len=0;
	unsigned		max=0;
	bool 			retVal;
	int				corr;
	bool			flg;


	temp = (char*)malloc(64);
	copy = (char*)malloc(64);

	strcpy(copy, word);
	switch (GetTokenType(word, level))
	{
		case kIsPrefix:
			k++;
			retVal = true;
			break;

		case kIsWord:
			level++;
			retVal = true;
			break;
			
		case kNone:
			if (k>1)
				MidS(word, temp, 1, k-1);
			else
				strcpy(word, temp);
			tempRecPtr = TryLookAhead(CreateSelectionFromLevel(MaxLevelNumber), temp);
			
			//calcolo max
			basePtr = tempRecPtr;
			while (tempRecPtr != 0L)
			{
				MidS(word, word, 1, k-1);
				strcpy(temp, tempRecPtr->word+k-1);
				strcat(word, temp);
				
				len = strlen(word);
				if (len <= MaxApplLimit - masterPtr)
				{
					MidS(inBuffer, temp, masterPtr, len);
					corr = GetCorrectionNumber(word, temp);
					if (corr > 0)
						tempRecPtr->max = len / corr;
					else
						tempRecPtr->max = 0;
				}
				else
					tempRecPtr->max = 0;
				
				tempRecPtr = tempRecPtr->next;
			}
			
			*temp = 0;
			tempRecPtr = basePtr;
			
			// sostituzione
			while (tempRecPtr != 0L)
			{
				if (tempRecPtr->max == max)
					if (strlen(tempRecPtr->word) > strlen(temp))
						strcpy(temp, tempRecPtr->word);
				if (tempRecPtr->max > max)
				{
					max = tempRecPtr->max;
					strcpy(temp, tempRecPtr->word);
				}
				
				tempRecPtr = tempRecPtr->next;
			}
			
			/*if (*solutions[0] == 0)
				strcpy(solutions[0], temp);*/

			FreeListFromMemory(tempRecPtr);
			tempRecPtr = 0L;
			
			//2
			strcpy(word, copy);
			len = strlen(word);
			tempRecPtr = CreateSelectionFromLevel(MaxLevelNumber);
			basePtr = tempRecPtr;
			flg = false;
			while (tempRecPtr != 0L)
			{
				MidS(inBuffer, temp, masterPtr-len+1, strlen(tempRecPtr->word));
				corr = GetCorrectionNumber(tempRecPtr->word, temp);
				if (corr > 0)
				{
					tempRecPtr->max = strlen(tempRecPtr->word) / corr;
					flg = true;
				}
				else
					tempRecPtr->max = 0;
				
				tempRecPtr = tempRecPtr->next;
			}
			
			if (flg)
				{
				*temp = 0;
				tempRecPtr = basePtr;
				
				while (tempRecPtr != 0L)
				{
					if (tempRecPtr->max == max)
						if (strlen(tempRecPtr->word) > strlen(temp))
							strcpy(temp, tempRecPtr->word);
					if (tempRecPtr->max > max)
					{
						max = tempRecPtr->max;
						strcpy(temp, tempRecPtr->word);
					}
					
					tempRecPtr = tempRecPtr->next;
				}
				if (*solutions[1] == 0)
					strcpy(solutions[1], temp);
			}

			FreeListFromMemory(tempRecPtr);
			tempRecPtr = 0L;
			
			//3
			strcpy(word, copy);
			*(word+k-2) = *(word+k-1);
			*(word+k-1) = 0;
			
			strcpy(tempSol3rd, word);
			masterPtrStorage = masterPtr;
			
			//4
			if (*tempSol4th == 0)
			{
				strcpy(tempSol4th, copy);
			
				if (k==1)
				{
					*(tempSol4th+k)=*tempSol4th;
					*(tempSol4th+k+1)=0;
					*tempSol4th='+';
				}
				else
					InsChar(tempSol4th, k-1, '+');
				masterPtrStorage4 = masterPtr;
			}
			retVal = false;
	}

	free(temp);
	free(copy);
	return retVal;
}

unsigned Correct(char* word)
{
	unsigned    int	k = 1, currLevel=0;
	unsigned	oldk;
	char*		copy;
	unsigned	len;
	bool		exitFlag=false;
	
	copy = (char*)malloc(128);
	strcpy(copy, word);
	
	len = strlen(word);
	while ((k <= len) && (currLevel <= MaxLevelNumber) && (!exitFlag))
	{
		MidS(word, word, 1, k);
		oldk=k;
		if (Parse(word, k, currLevel) == false)
		{
			exitFlag = true;
			currLevel = MaxLevelNumber+1;
		}
		strcat(word, copy+oldk);
	}
	
	free(copy);
	return currLevel;
}



bool AreEnoughSimilar(char *word1, char *word2)
{
	register short i;
	register short p = 0;
	register short perc;
	unsigned short	len;

	// 'percentuale di somiglianza'
	// 75% sembra essere un buon valore
	const unsigned PercS=75;

	len = strlen(word1);
	if (len != strlen(word2)) // le stringhe devono essere della stessa lunghezza
		return false;

	perc = 100 / len;

	for (i=0; i<=len; i++)
		if (*(word1+i) == *(word2+i))
			p += perc;

	if (p > PercS)
		return true;
	else
		return false;
}

// cerca le stringhe somiglianti
// usando AreEnoughSimilar
RecordListPtr TryMaxLikelihood(RecordListPtr theSelection, char *theWord)
{
	RecordListPtr	resultSelection = NULL;
	unsigned		solutionCount = 0;

	while ((solutionCount <= kMaxSolutions) && (theSelection != NULL))
	{
		if (AreEnoughSimilar(theWord, theSelection->word))
		{
			AppendRecord(resultSelection, theSelection->word);
			solutionCount++;
		}
		theSelection = theSelection->next;
	}
	
	return resultSelection;
}

// ritorna vero se la stringa where
// inizia con what
bool StartsWith(char *what, char *where)
{	
	register short i;
	register short f = 0;
	unsigned short len;

	len = strlen(what);
	if (len > strlen(where)) // "where" deve essere maggiore o uguale a "what"
		return false;

	for (i=0; i<len; i++)
		if (*(where+i) == *(what+i))
			f++;
	if (f == len)
		return true;
	else
		return false;
}

// implementazione di ricerca del look-ahead
// usando StartsWith
RecordListPtr TryLookAhead(RecordListPtr theSelection, char *theWord)
{
	RecordListPtr	resultSelection = NULL;
	unsigned		solutionCount = 0;

	while (theSelection != NULL)
	{
		if (StartsWith(theWord, theSelection->word))
		{
			AppendRecord(resultSelection, theSelection->word);
			solutionCount++;
		}
		theSelection = theSelection->next;
	}
	
	return resultSelection;
}

// se le stringhe sono uguali...
RecordListPtr FindTheSameWord(RecordListPtr theSelection, char *theWord)
{
	RecordListPtr	resultSelection = NULL;
	bool			foundFlag = false;

	while (!foundFlag && (theSelection != NULL))
	{
		if (!strcmp(theWord, theSelection->word))
		{
			AppendRecord(resultSelection, theSelection->word);
			foundFlag = true;
		}
		theSelection = theSelection->next;
	}

	return resultSelection;
}


// costruisce una lista linkata di record
void AppendRecord(RecordListPtr &curr, char* s)
{
	if (curr == NULL)
	{
		curr = new record;
		strcpy(curr->word, s);
		curr->next = NULL;
	}
	else
		AppendRecord(curr->next, s);
}

// genera le combinazioni di un livello
// in base a quelle del livello precedente
void MakeLevel(int	lev, int index, int source)
{
	char	*c;

	if (lev>0)
	{	
		c = liv[lev].GetString(index);
		strcat(liv[lev].GetString(index), liv[lev-1].word[source]);
	}
}

// lettura file codici.txt
int ReadFile()
{
	FILE	*myfile;
	unsigned		n, np;

	unsigned	h=1;
	char	c;
	unsigned		i;

	myfile = fopen("codici.txt", "r");
	if (myfile == NULL)
		return kFileErr;

	fscanf(myfile, "%d", &AlphabethCharNumber);
	fscanf(myfile, "%c", &c);

	for (i=0; i<AlphabethCharNumber; i++)
	{
		fscanf(myfile, "%c", &c);
		liv[0].SetString(i, &c, 1);
	}

	liv[0].wordn = AlphabethCharNumber;
	fscanf(myfile, "%c", &c);
	fscanf(myfile, "%d", &MaxLevelNumber);
	
	while (h<=MaxLevelNumber)
	{
		fscanf(myfile, "%d", &np); //numero parole per livello
		liv[h].wordn = np;

		for (i=0; i<np; i++)
		{
			fscanf(myfile, "%d", &n);
			while (n != 0)
			{
				MakeLevel(h, i, n-1);
				fscanf(myfile, "%d", &n);
			}
		}
		h++;
	}

	cout<<"\ncodici.txt letto con successo.\n";
	fclose(myfile);

	return 0;
}

void printfile()
{
	unsigned i;

	// solo debugging
	cout<<AlphabethCharNumber<<"\n";

	for (i=0; i<AlphabethCharNumber; i++)
		cout<<liv[0].GetString(i);

	cout<<"\n";
	for (i=0; i<3; i++)
		cout<<liv[1].GetString(i)<<" ";

}

// scrittura file codici.txt
void WriteFile()
{
	FILE	*myfile;
	int		n, np, mc, nl;
	int		c;
	int		i;

	printf("\n\nCREAZIONE FILE CODICI\n\n");
	myfile = fopen("codici.txt", "w");
	printf("Numero caratteri alfabeto? ");
	scanf("%d", &mc);
	getchar();

	fprintf(myfile, "%d", mc);
	fprintf(myfile, "\n"); // scrivi cr
	printf("\nInserisci la stringa dei caratteri dell'alfabeto:");
	for (i=0; i<mc; i++)
	{
		c=getchar();
		fprintf(myfile, "%c", c);
	}

	c=getchar();
	fprintf(myfile, "\n"); // scrivi cr
	printf("\nNumero di livelli?");
	scanf("%d", &nl);
	fprintf(myfile, "%d", nl);
	fprintf(myfile, "\n"); // scrivi cr

	for (i=0; i<nl; i++)
	{
		printf("Numero parole livello %d ?", i+1);
		scanf("%d", &np);
		fprintf(myfile, "%d", np);
		fprintf(myfile, "\n"); // scrivi cr
		
		for (int j=0; j<np; j++)
		{
			printf("Livello <%d> Parola n. %d (termina con zero)\n", i+1, j+1);
			n = -1;
			while (n != 0)
			{
				scanf("%d", &n);
				fprintf(myfile, "%d", n);
				fprintf(myfile, "\n"); // scrivi cr
			}
		}
	}

	fclose(myfile);
	printf("\nFile codici.txt creato con successo.\n\n");
}

short OpenDataStream(char* nomefile, FILE* &f)
{	
	f = fopen(nomefile, "rb");
	if (f == 0L)
		return kFileErr;
	else
		return no_Err;
}

short CloseDataStream(FILE* f)
{
	fclose(f);
	return no_Err;
}

short GetCharFromStream(FILE* f, char &ch)
{
	if (fread(&ch, 1, 1, f) == 0)
		return kFEOF;
	else
		return no_Err;
}

void AppendCharToCStr(char c, char* s)
{
	char*	t;
	
	t = (char*)malloc(2);
	*t = c;
	*(t+1) = 0;
	strcat(s, t);
	free(t);
}

void printhelp()
{
	cout<<"Utilizzo:\nparse [-n]\nOpzioni:\n -n  Crea un nuovo file di codici\n\n";
}

void newcodes(char *argv[])
{
	if ((!strcmp(argv[1], "-n")) || (!strcmp(argv[1], "-N")))
		WriteFile();
	else
		printhelp();
}

int LoadBuffer(char* bufferPtr)
{
	int		i=0;
	char	ch;
	
	while ((GetCharFromStream(theStream, ch) != kFEOF))
	{
		AppendCharToCStr(ch, bufferPtr);
		i++;
	}
	return i;
}

int main(int argc, char *argv[])
{
	int				theErr;
	char			ch, choice;
	char*			copy;
	long			mptrrec;
	int				min, index, tempmax;
	bool			f=false;
	char*			a;
	RecordListPtr	tempRecPtr=0L;
	char*			b;
	unsigned		i, max;
	char*			outBuffer;
	char*			source;

	source = (char*)malloc(2048);
	outBuffer = (char*)malloc(2048);
	inBuffer = (char*)malloc(2048);
	theWord = (char*)malloc(64);
	copy = (char*)malloc(64);
	tempSol3rd = (char*)malloc(64);
	tempSol4th = (char*)malloc(64);
	a = (char*)malloc(64);
	b = (char*)malloc(64);
	for (i=0; i<4; i++)
	{
		solutions[i] = (char*)malloc(64);
		*solutions[i]=0;
	}
	*source = 0;
	*inBuffer = 0;
	*outBuffer = 0;
	*tempSol3rd = 0;
	*tempSol4th = 0;
	masterPtr = 0;
	
	// controllo parametri linea comandi
	if (argc == 2)
		newcodes(argv);
	else if (argc > 2)
	{
		printhelp();
		return kBadCommandLine;
	}

	FINE = fopen("fine.txt", "w");

	if ((theErr = ReadFile()) != no_Err)
	{
		cout<<"Errore nella lettura del file CODICI.TXT\nAssicurati che sia nella stessa directory del programma e\nche non sia corrotto.\n\n";
		return theErr;
	}

	cout<<"\nParse Correttore\n\n";
	
	cout<<"Vuoi la correzione automatica (s/n)?";
	cin>>choice;
	autoFlag = (choice == 'S');
	

	if (OpenDataStream(theFile, theStream) != no_Err)
		return kFileErr;
	MaxApplLimit = LoadBuffer(inBuffer);
	strcpy(source, inBuffer);
	while (masterPtr < MaxApplLimit)
	{
		for (i=0; i<4; i++)
			*solutions[i]=0;
		*theWord = 0;
		*copy = 0;
		masterPtrStorage = 0;
		while ((masterPtr < MaxApplLimit) && (Correct(theWord) <= MaxLevelNumber))
		{
			masterPtr++;
			ch = *(inBuffer+masterPtr-1);
			if (CharBelongsToOurAlphabeth(ch))
			{
				AppendCharToCStr(ch, theWord);
				AppendCharToCStr(ch, copy);
			}
			else
				cout<<"***Carattere errato: "<<ch<<"\n";
		}
		// terza strategia
		if (masterPtrStorage != 0)
		{
			mptrrec = masterPtr;
			masterPtr = masterPtrStorage;
			
			strcpy(theWord, tempSol3rd);
			while ((masterPtr < MaxApplLimit) && (Correct(theWord) <= MaxLevelNumber))
			{
				masterPtr++;
				ch = *(inBuffer+masterPtr-1);
				if (CharBelongsToOurAlphabeth(ch))
					AppendCharToCStr(ch, theWord);
				else
					cout<<"***Carattere errato: "<<ch<<"\n";
			}
			
			if (FindTheSameWord(CreateSelectionFromLevel(MaxLevelNumber), theWord) != 0L)
				strcpy(solutions[2], theWord);
			masterPtr = mptrrec;
		}
		if ((*solutions[2] == 0) && ItCanBeAnotherWord(theWord))
		{
			mptrrec = masterPtr;
			masterPtr = masterPtrStorage;
			
			if (*solutions[1] != 0L)
				strcpy(theWord, solutions[0]);
			else
				strcpy(theWord, solutions[1]);
			while ((masterPtr < MaxApplLimit) && (Correct(theWord) <= MaxLevelNumber))
			{
				masterPtr++;
				ch = *(inBuffer+masterPtr-1);
				if (CharBelongsToOurAlphabeth(ch))
					AppendCharToCStr(ch, theWord);
				else
					cout<<"***Carattere errato: "<<ch<<"\n";
			}
			
			if (FindTheSameWord(CreateSelectionFromLevel(MaxLevelNumber), theWord) != 0L)
				strcpy(solutions[2], theWord);
			masterPtr = mptrrec;
		}
		
		// Quarta strategia
		tempRecPtr = 0L;
		max = 0;
		for (i=0; i<AlphabethCharNumber; i++)
		{
			unsigned len;
			strcpy(theWord, tempSol4th);
			len = strlen(theWord);
			mptrrec = masterPtr;
			masterPtr = masterPtrStorage4;
			*(theWord+len-2) = *liv[0].GetString(i);
			
			while ((masterPtr < MaxApplLimit) && (Correct(theWord) <= MaxLevelNumber))
			{
				masterPtr++;
				ch = *(inBuffer+masterPtr-1);
				if (CharBelongsToOurAlphabeth(ch))
					AppendCharToCStr(ch, theWord);
				else
					cout<<"***Carattere errato: "<<ch<<"\n";
			}
			
			if (FindTheSameWord(CreateSelectionFromLevel(MaxLevelNumber), theWord) != 0L)
			{
				AppendRecord(tempRecPtr, theWord);
				tempRecPtr->ch = *liv[0].GetString(i);
			}
			else
			{
				*(theWord+strlen(theWord)-1)=0;
				if (FindTheSameWord(CreateSelectionFromLevel(MaxLevelNumber), theWord) != 0L)
				{
					AppendRecord(tempRecPtr, theWord);
					tempRecPtr->ch = *liv[0].GetString(i);
				}
			}
			masterPtr = mptrrec;
		}
		// max
		*b = 0;
		max = 0;
		while (tempRecPtr != 0L)
		{
			MidS(inBuffer, a, masterPtr-strlen(tempSol4th)+2, strlen(theWord));
			InsChar(a, strlen(copy)-1, tempRecPtr->ch);
			
			tempmax = GetCorrectionNumber(a, tempRecPtr->word);
			if (tempmax >= 0)
				tempRecPtr->max = strlen(a) / tempmax;
			else if (tempmax == 0)
				tempRecPtr->max = 32767;
			
			if (tempRecPtr->max > max)
			{
				max = tempRecPtr->max;
				strcpy(b, tempRecPtr->word);
			}
			else if (tempRecPtr->max == max)
				if (strlen(tempRecPtr->word) > strlen(b))
					strcpy(b, tempRecPtr->word);
			
			tempRecPtr = tempRecPtr->next;
		}
				
		FreeListFromMemory(tempRecPtr);
		if (*b != 0)
			strcpy(solutions[3], b);
		
		cout<<"  "<<copy<<"\n";
		if (GetTokenType(copy, MaxLevelNumber) == kIsWord)
		{
			cout<<"<OK>\n";
			fprintf(FINE, "\n%s", copy);
			strcat(outBuffer, copy);
		}
		else
		{
			f = false;
			for (i=0; i<4; i++)
				if (*solutions[i] != 0L)
					f = true;
			
			if (f)
			{
				cout<<"1)"<<solutions[0]<<"\n";
				cout<<"2)"<<solutions[1]<<"\n";
				cout<<"3)"<<solutions[2]<<"\n";
				cout<<"4)"<<solutions[3]<<"\n";
				fprintf(FINE, "1)%s\n", solutions[0]);
				fprintf(FINE, "2)%s\n", solutions[1]);
				fprintf(FINE, "3)%s\n", solutions[2]);
				fprintf(FINE, "4)%s\n", solutions[3]);
				// scelta delle soluzioni
				if (autoFlag)
				{
					max = 0;
					min = 32767;
					index = -1;
					for (i=0; i<4; i++)
					{
						if (*solutions[i] != 0)
						{
							MidS(inBuffer, a, masterPtr-strlen(copy)+1, strlen(solutions[i]));
							if (i == 2)
							{
								MidS(inBuffer, a, masterPtr-strlen(copy)+1, strlen(solutions[i])+1);
								StripChar(a, strlen(copy) - 1);
							}
							else if (i == 3)
							{
								MidS(inBuffer, a, masterPtr-strlen(copy)+1, strlen(solutions[i])+1);
								InsChar(a, strlen(copy) - 1, ch);
							}

							tempmax = GetCorrectionNumber(solutions[i], a);
							if (tempmax >= 0)
							{
								if (tempmax < min)
								{
									min = tempmax;
									index = i;
								}
								else if (tempmax == min)
									if (strlen(solutions[i]) > strlen(solutions[index]))
										index = i;
							}
						}
					}
					if (index >= 0)
					{
						cout<<"Scelgo la n. "<<index+1<<"\n";
						fprintf(FINE, "Scelta n.%i\n", index+1);
					}
					else
					{
						cout<<"Forzo la n. 1\n";
						fprintf(FINE, "Forzo n.1\n");
					}
					cout<<"\nPremi un tasto e poi INVIO...";
					cin>>ch;
				}
				else // manual
				{
					cout<<"Scegli #:";
					cin>>index;
					index--;
				}
				if (index < 0)
				{
					index = 0;
					if (strlen(a) < strlen(solutions[0]))
					{
						masterPtr -= strlen(copy)+1;
						masterPtr += strlen(solutions[0])-1;
					}
				}
				else
				{
					if (*solutions[index] == 0)
							if (*solutions[0] != 0)
								index = 0;
							else if (*solutions[1] != 0)
								index = 1;
							else if (*solutions[2] != 0)
								index = 2;
							else if (*solutions[3] != 0)
								index = 3;
					masterPtr -= strlen(copy)+1;
					masterPtr += strlen(solutions[index]);
				}
				if (index == 2)
				{
					StripChar(inBuffer, masterPtrStorage-1);
					MaxApplLimit--;
				}
				else if (index == 3)
				{
					InsChar(inBuffer, masterPtrStorage-1, ch);
					MaxApplLimit++;
				}
				masterPtr++;
				strcat(outBuffer, solutions[index]);
			}
			else
			{
				cout<<"Nessuna soluzione trovata.\n";
				fprintf(FINE, "Nessuna soluzione\n");
			}
		}
	}
	cout<<"\n IN:\n"<<source<<"\nOUT:\n"<<outBuffer;
	cout<<"\nEOF\n";
	fprintf(FINE, "\nIN\n%s", source);
	fprintf(FINE, "\nOUT\n%s", outBuffer);

	
	CloseDataStream(theStream);
/*	free(a);
	free(theWord);
	free(copy);
	free(tempSol3rd);
	free(tempSol4th);
	free(inBuffer);
*/
	fclose(FINE);
	return no_Err;
}

bool ItCanBeAnotherWord(char* ts)
{
	RecordListPtr	tempRecPtr;
	unsigned			ml=32767;
	unsigned long			r;
	
	tempRecPtr = CreateSelectionFromLevel(MaxLevelNumber);
	while (tempRecPtr != 0L)
	{
		if (strlen(tempRecPtr->word) < ml)
			ml = strlen(tempRecPtr->word);
		tempRecPtr = tempRecPtr->next;
	}
	
	r = MaxApplLimit - masterPtr - strlen(ts);
	if (r >= ml)
		return true;
	else
		return false;
}
