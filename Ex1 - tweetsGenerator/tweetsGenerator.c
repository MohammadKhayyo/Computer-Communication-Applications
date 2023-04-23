
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_WORDS_IN_SENTENCE_GENERATION 20
#define MAX_WORD_LENGTH 100
#define MAX_SENTENCE_LENGTH 1000

typedef struct WordStruct {
    char *word;
    struct WordProbability *prob_list;
    int frequency;
    int SizeOfProb_list;
} WordStruct;

typedef struct WordProbability {
    struct WordStruct *word_struct_ptr;
    int frequency;
} WordProbability;

/************ LINKED LIST ************/
typedef struct Node {
    WordStruct *data;
    struct Node *next;
} Node;

typedef struct LinkList {
    Node *first;
    Node *last;
    int size;
} LinkList;

int StringToInt(char num[]) // this method turn the String to int
{
    int size = (int) strlen(num);
    int i, counter = 1, number = 0;
    for (i = size - 1; i >= 0; i--) {
        number += ((num[i] - '0') * counter);
        counter *= 10;
    }
    return number;
}
/**
 * Add data to new node at the end of the given link list.
 * @param link_list Link list to add data to
 * @param data pointer to dynamically allocated data
 * @return 0 on success, 1 otherwise
 */
int add(LinkList *link_list, WordStruct *data)
{
    Node *new_node = malloc(sizeof(Node));
    if (new_node == NULL)
    {
        return 1;
    }
    *new_node = (Node){data, NULL};

    if (link_list->first == NULL)
    {
        link_list->first = new_node;
        link_list->last = new_node;
    }
    else
    {
        link_list->last->next = new_node;
        link_list->last = new_node;
    }
    link_list->size++;
    return 0;
}
/*************************************/
WordStruct* FindInDictionary(LinkList *dictionary, char* A){//to find a given word in link list, and return new one if not found
    if(dictionary!=NULL&&dictionary->first!=NULL&&dictionary->first->data!=NULL) {
        Node *check = dictionary->first;
        while (check != NULL) {
            if (strcmp(check->data->word, A) == 0) {
                check->data->frequency += 1;
                free(A);
                return check->data;
            }
            check = check->next;
        }
    }
    WordStruct* data= (WordStruct*) malloc(sizeof (WordStruct));
    if(data==NULL){
        fprintf(stdout,"Allocation failure: ");
        exit(EXIT_FAILURE);
    }
    data->prob_list=NULL;
    data->SizeOfProb_list=0;
    data->word=A;
    data->frequency=1;
    add(dictionary , data);
    return data;
}
/**
 * Get random number between 0 and max_number [0, max_number).
 * @param max_number
 * @return Random number
 */
int get_random_number(int max_number)
{
    int random_number=rand();
    if(random_number==0) return 0;
    return random_number%max_number;
}
 int SizeOfLinkList (LinkList *dictionary){//calculate the length of link list
    if(dictionary==NULL||dictionary->first==NULL||dictionary->first->data==NULL) return 0;
    Node* Curr = dictionary->first;
    int size = 0;
    while(Curr != NULL){
        Curr = Curr->next;
        size++;
    }
    return size;
 }
/**
* Choose randomly the next word from the given dictionary, drawn uniformly.
* The function won't return a word that end's in full stop '.' (Nekuda).
* @param dictionary Dictionary to choose a word from
* @return WordStruct of the chosen word
 * randomly select a word from the link list that doesn't end in a dot.
*/
WordStruct *get_first_random_word(LinkList *dictionary)
{
    Node* node;int temp;
    while(1){
        for(node=dictionary->first,temp=get_random_number(dictionary->size);temp!=0;node=node->next,temp--);
        if(node!=NULL&&node->data!=NULL&&node->data->prob_list!=NULL) break;
    }
    return node->data;
}
/**
 * Choose randomly the next word. Depend on it's occurrence frequency
 * in word_struct_ptr->WordProbability.
 * @param word_struct_ptr WordStruct to choose from
 * @return WordStruct of the chosen word
 * randomly select a word after the given word from link list according to the probability (frequency)
 */
WordStruct *get_next_random_word(WordStruct *word_struct_ptr)
{
        int Index,TheSizeOfProb_List=word_struct_ptr->SizeOfProb_list,RandomFrequency=get_random_number(word_struct_ptr->frequency);//Random Frequency
            for(Index=0; Index < TheSizeOfProb_List; Index++){
             RandomFrequency-=word_struct_ptr->prob_list[Index].frequency;
                if(RandomFrequency < 0) return word_struct_ptr->prob_list[Index].word_struct_ptr;
        }
        return word_struct_ptr->prob_list[word_struct_ptr->SizeOfProb_list-1].word_struct_ptr;
}
/**
 * Receive dictionary, generate and print to stdout random sentence out of it.
 * The sentence most have at least 2 words in it.
 * @param dictionary Dictionary to use
 * @return Amount of words in printed sentence
 * generate random sentence by calling other function
 */
int generate_sentence(LinkList *dictionary)
{
    WordStruct* Word1 = get_first_random_word(dictionary);
    fprintf(stdout,"%s ",Word1->word);
    int HowManyWords;
    WordStruct *Word2;
    for(HowManyWords=0; HowManyWords < (MAX_WORDS_IN_SENTENCE_GENERATION-1); HowManyWords++){
        Word2= get_next_random_word(Word1);
        fprintf(stdout,"%s",Word2->word);
        if(Word2->prob_list==NULL) break;
        else printf(" ");
        Word1=Word2;
    }
    printf("\n");
    return HowManyWords;
}
/**
 * Gets 2 WordStructs. If second_word in first_word's prob_list,
 * update the existing probability value.
 * Otherwise, add the second word to the prob_list of the first word.
 * @param first_word
 * @param second_word
 * @return 0 if already in list, 1 otherwise.
 * keeps track of the word coming after the first word.
 */
int add_word_to_probability_list(WordStruct *first_word,
                                 WordStruct *second_word)
{
    if(first_word->prob_list == NULL){
        first_word->prob_list = (WordProbability*) malloc(sizeof(WordProbability));
        if(first_word->prob_list==NULL){
            fprintf(stdout,"Allocation failure: ");
            exit(EXIT_FAILURE);
        }
        first_word->SizeOfProb_list=1;
    }else{
        for (int i = 0; i < first_word->SizeOfProb_list ; ++i) {
            if(strcmp(first_word->prob_list[i].word_struct_ptr->word,second_word->word) == 0){
                first_word->prob_list[i].frequency++;
                return 0;
            }
        }
        first_word->SizeOfProb_list++;
        first_word->prob_list = (WordProbability*) realloc(first_word->prob_list ,(first_word->SizeOfProb_list)*sizeof(WordProbability));
    }
    first_word->prob_list[first_word->SizeOfProb_list-1].frequency=1;
    first_word->prob_list[first_word->SizeOfProb_list-1].word_struct_ptr = second_word;
    return 1;
}
int howManyWords(const char* str); // the method will calculate how many words we entered through the string
char** Sentences_Splitter(char** WordInSentence, char str[]);// this method will cut the sentence into words
/**
 * Read word from the given file. Add every unique word to the dictionary.
 * Also, at every iteration, update the prob_list of the previous word with
 * the value of the current word.
 * @param fp File pointer
 * @param words_to_read Number of words to read from file.
 *                      If value is bigger than the file's word count,
 *                      or if words_to_read == -1 than read entire file.
 * @param dictionary Empty dictionary to fill
 */
void fill_dictionary(FILE *fp, int words_to_read, LinkList *dictionary)
{
    char* String1=(char*)malloc(sizeof (char ) * (MAX_SENTENCE_LENGTH + 1));
    char* String2=(char*)malloc(sizeof (char ) * 2);
    if(String1 == NULL || String2 == NULL){
        fprintf(stdout,"Allocation failure: ");
        exit(EXIT_FAILURE);
    }
    strcpy(String1, "");
    strcpy(String2, "");
    int Save_word_to_read=words_to_read;
    while(fgets(String1, MAX_SENTENCE_LENGTH, fp) != NULL) {//reads from the file's
        if(strcmp(String1, "\n") == 0) continue;
        if(String1[strlen(String1) - 1] == '\n' ) {
            String1[strlen(String1) - 1] = '\0';
        }
        if(words_to_read!=-1&&Save_word_to_read<=0) break;
        if(words_to_read!=-1)
            Save_word_to_read-= howManyWords(String1);
        String2=(char *) realloc(String2, sizeof (char ) * (strlen(String1) + strlen(String2) + 1 + 1));//resize the string to contain another line
        if(String2 == NULL){
            fprintf(stdout,"Allocation failure: ");
            exit(EXIT_FAILURE);
        }
        strcat(String2, String1);
        strcat(String2, " ");
    }
    int How_Many_Word_We_Read=howManyWords(String2);
    char** AllTheWord =(char **) malloc(sizeof (char *) * How_Many_Word_We_Read);
    Sentences_Splitter(AllTheWord, String2);//fill the word in the array.
    if(AllTheWord==NULL){
        fprintf(stdout,"Allocation failure: ");
        exit(EXIT_FAILURE);
    }
    free(String2);
    WordStruct* Curr;
    WordStruct* Prev;
    int i;
    int IsNotFirstWordInTheSentence=0;
        for(i=0; i < How_Many_Word_We_Read; i++){
            if(words_to_read!=-1&&(words_to_read--)<=0) {
                for(; i < How_Many_Word_We_Read; i++){
                    free(AllTheWord[i]);//free unused words
                }
                break;
            }
            Curr = FindInDictionary(dictionary, AllTheWord[i]);//search for the word in the linked list
            if (IsNotFirstWordInTheSentence != 0) {
                add_word_to_probability_list(Prev, Curr);
            }
            IsNotFirstWordInTheSentence=1;
            if(Curr->word[strlen(Curr->word)-1]=='.') IsNotFirstWordInTheSentence=0;
            Prev = Curr;
        }
    free(AllTheWord);
    free(String1);
    dictionary->size=SizeOfLinkList (dictionary);
}
int howManyWords(const char* str) { // the method will calculate how many words we entered through the string
    int i;
    int countOfWords = 0;
    for (i = 0; str[i] != '\0'; i++) {// this Loop count how many words in the String
        if (str[i] != ' ' && (str[i+1] == ' ' || str[i+ 1] == '\0')) {// this condition checks  if the char is not a space and the char after it is a space or the end of sentence
            countOfWords++;
        }
    }
    return countOfWords;
}
char** Sentences_Splitter(char** WordInSentence, char* str) {// this method will cut the sentence into words
    char *token = strtok(str, " ");
    int Index=0;
    while(token!=NULL){
        if(strcmp(token," ")==0){
            token= strtok(NULL," ");
            continue;
        }
        WordInSentence[Index]=(char *) malloc(sizeof (char )*(MAX_WORD_LENGTH +1));//Allocation memory
            if(WordInSentence[Index]==NULL) {
                fprintf(stdout,"Allocation failure: ");
                exit(EXIT_FAILURE);
            }
            WordInSentence[Index]=strcpy(WordInSentence[Index],token);
            Index++;
            token= strtok(NULL," ");
    }
    return WordInSentence;
}
/**
 * Free the given dictionary and all of it's content from memory.
 * @param dictionary Dictionary to free
 */
void free_dictionary(LinkList *dictionary)
{
    Node* Curr=dictionary->first;
    Node* Prev;
        while (Curr!=NULL){//First run on linked list and frees probability list
            if(Curr->data->prob_list!=NULL) {
                free(Curr->data->prob_list);
            }
            Curr=Curr->next;
        }
        Curr=dictionary->first;
    while(Curr!=NULL){//Secound run on linked list
        free(Curr->data->word);
        free(Curr->data);
        Prev=Curr;
        Curr=Curr->next;
        free(Prev);
    }
        free(Curr);
        free(dictionary);
}
/**
 * @param argc
 * @param argv 1) Seed
 *             2) Number of sentences to generate
 *             3) Path to file
 *             4) Optional - Number of words to read
 */
int main(int argc, char *argv[])
{
    if(argc<4||argc>5){
        fprintf(stdout,"Usage: Seed <int>,Number of sentences to generate<int>,Path to file<String>,Optional - Number of words to read<int>");
        exit(EXIT_FAILURE);
    }
    srand(StringToInt(argv[1]));
    int How_Many_Sentence_To_Make= StringToInt(argv[2]);
    FILE * fp = fopen(argv[3], "r");
    if(fp==NULL){
        fprintf(stdout, "Error: count open file");
        exit(EXIT_FAILURE);
    }
    int Word_To_Read=-1;
    if(argc==5) Word_To_Read=StringToInt(argv[4]);
    LinkList *dictionary=(struct LinkList*)malloc(sizeof(struct LinkList));//creat link list
    if(dictionary==NULL){
        fprintf(stdout,"Allocation failure: ");
        exit(EXIT_FAILURE);
    }
    dictionary->first=NULL;
    fill_dictionary(fp, Word_To_Read, dictionary);
        for(int i=0; i < How_Many_Sentence_To_Make; i++) {//generates a certain number of sentences requested from the user
            printf("Tweet %d: ",i+1);
            generate_sentence(dictionary);
        }
    free_dictionary(dictionary);
    fclose(fp);// Close File
    return 0;
}
