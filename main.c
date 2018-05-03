#include <stdio.h>
#include <ntsid.h>
#include <memory.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/time.h>
#include <ctype.h>

#define MAX_LEN 256

//Let our array grow by 10 Files at a time
#define GROWBY 10

#define VALID_CONFIRMATION_FILE_LENGTH 17
#define VALID_TIMESTAMP_LENGTH 10
#define TRANSACTIONS_CONFRIMED_PATH "TransactionsConfirmed"
#define TRANSACTIONS_PENDING_PATH "TransactionsPending"
#define USER_ADDRESS "23bd14810c04739b16Ee5a835114C13A8A2C6166"


enum transactionType{send, receive};

typedef struct Tran {

    time_t fileName;
    char recipientAddress[256];
    float amount;
    enum transactionType type;
} Transaction;

typedef struct recip {
    char address[50];
    char label[40];
} Recipient;

void menuText();
int display_menu (char ** transitionedFileList, int noOfNotification);

void load_data (char ** confirmedFileList, int confirmedCount,char **pendingFileList, int pendingCount);

// Transactions related Functions
void initTransaction (Transaction *tranPtr, time_t fileName, char recipientAddress[], float amount, int type);
void loadTransactions (Transaction *tranPtr[], char * filenames[], char *path, int numberOfFiles);
void viewTransactions ();
void printTransactions (Transaction *ptr[],  char * status, char * transactionType, int numberOfFiles);

// Functions for amounts
void calculateTotals ();
void viewCoinBalance ();
float * calculateTotalPerStatus (Transaction *ptr[],  int size , char * status);
void printTotals (char * status, float resultBuffer[]);

// Recipient functions
void addNewRecipient ();
void loadRecipients();
void initRecipient(Recipient *rec, char address[], char label[]);
void tokeniseStringToArray(char *tokenArray[], char strTempData[]);

void sendCoin();

// Sorting Functions
int sortTransactions();
void bubbleSortTransactions(Transaction *array[], int arraySize, int sortBy);
void sortByTransactionType(Transaction *sortingArray[], Transaction *sourceArray[], int arraySize, int sortBy, char * status);

void exitMCW();

void validInput(char *choice, int limit);

// Notification functions
void storePendingTransactionInfo();
int readPendingTransactionInfo();
char ** checkNotifications(int pendingFromLastSession, char ** validFileList, int count, int * noOfNotifications);

// Part B Bits
typedef enum { INVALID_UNKNOWN, INVALID_DOT, INVALID_LENGTH, INVALID_SUFFIX, INVALID_NONNUMERIC, INVALID_FUTURETIME, VALID } FileType;
char suffix[] = "MCW.txt";

FileType check_valid_filename(const char *filename, char *pszValidtimestamp);
char ** find_valid_transaction_files(int * confirmed_transactions_count, int fileType);


static Transaction *pendingTransPtr[MAX_LEN], *confirmedTransPtr[MAX_LEN]; // Arrays to hold pointers to transactions
static Recipient *recipients[MAX_LEN]; // Array of pointer to recipients.
static int pendingSize, confirmedSize, noOfRecipients = 0; // Sizes of the transaction & recipient arrays.
static char *type[] = {"All", "Sent", "Received"}, *status[] = {"confirmed", "pending"}; // Transaction types and direction.
static float confirmedBalance = 0, pendingBalance = 0;  // Variables for holding amounts.
char *pendingBuffer[MAX_LEN]; // Array to hold pending transaction were moved to confirmed

int main() {
    int confCount = 0, pendCount = 0; //Number of valid files
    char **confirmedfilelist; //Pointer to memory buffer where the list of valid confirmation files will be kept.
    char **pendingFileList; //Pointer to memory buffer where the list of transitioned files will be kept.
    char **transitionedFileList; //Pointer to memory buffer where the list of transitioned files will be kept.

    int noOfNotifications = 0, pendingFromLastSession = 0; // No of transitions

    confirmedfilelist = find_valid_transaction_files(&confCount, 0);
    pendingFileList = find_valid_transaction_files(&pendCount, 1);

    // Load tranaction data
    load_data(confirmedfilelist, confCount, pendingFileList, pendCount);

    // Load in the pending session from previous session and see if there's a differnce with whats in the confirmed directory now.
    pendingFromLastSession = readPendingTransactionInfo();
    transitionedFileList = checkNotifications(pendingFromLastSession, confirmedfilelist, confCount, &noOfNotifications);

    display_menu(transitionedFileList, noOfNotifications);

    // Free up memory when exiting
    free(confirmedfilelist);
    free(pendingFileList);
    free(transitionedFileList);
    return 0;
}

/*
 * Small helper function to printout the main menu.
 */
void menuText(){

    puts ("\n*********\nMain Menu\n*********\n\nPlease enter the number of your choice.\n\n1. View Transactions\n2. Calculate Total Sent and Received\n3. View Wallet Balances\n4. Add New Recipient"
                  "\n5. Send Coin\n6. Sort Transactions\n7. Check notifications \n8. Exit Wallet");
};

/*
 * Small helper function to check if input is valid.
 * Useful for picking options from a list.
 */

void validInput(char *choice, int limit){

    while (!isdigit(*choice) || *choice == 10 || *choice > limit)
    {
        puts("Invalid menu option. Please try again.");
        fgets(choice, 8, stdin);
    }

};

/*
 * Main function to display the main menu
 * Has several branches for each of the main functionalities of the app.
 */

int display_menu (char ** transitionedFileList, int noOfNotification) {

    char choice[1];
    puts("*************************************\n*\t\t\t\t\t\t\t\t\t*\n*\t\tWelcome to your Wallet\t\t*\n*\t\t\t\t\t\t\t\t\t*\n*************************************");

    printf("\nYou have %d new transaction status changed. Check notifications for details...\n", noOfNotification);

    menuText();
    int notifications = noOfNotification;
    while (fgets(choice, 8, stdin)!= NULL){
        validInput(choice, 56);
        switch(choice[0]) {

            case 49 :
                printf("\nView Transactions\n\n");
                viewTransactions();
                menuText();
                break;
            case 50 :
                printf("\nCalculate Total Sent and Received\n\n");
                calculateTotals();
                menuText();
                break;
            case 51 :
                printf("\nView Wallet Balances\n\n");
                viewCoinBalance();
                menuText();
                break;
            case 52 :
                printf("\nAdd New Recipient\n\n");
                addNewRecipient();
                menuText();
                break;
            case 53 :
                printf("\nSend Coin\n");
                sendCoin();
                menuText();
                break;
            case 54 :
                printf("\nSort Transaction\n");
                sortTransactions();
                menuText();
                break;
            case 55 :
                printf("\nYou have %d new transaction status changed. See below for details...\n\n", notifications);
                // Print out all the necessary notifacations
                for (int i = 0; i < notifications; ++i) {
                    printf("Filename %s has gone from pending to confirmed.\n", transitionedFileList[i] );
                }
                menuText();
                break;
            case 56 :
                exitMCW();
                break;
            default :
                menuText();
        }
    }
    return 0;
};

/*
* This function initialises the Transaction structure for storage in the tranPtr variable.
* Has inputs for filename, recipient address, amount and the transaction type.
*/

void initTransaction(Transaction *tranPtr, time_t fileName, char recipientAddress[], float amount, int type) {
    tranPtr->fileName = fileName;
    strncpy(tranPtr->recipientAddress, recipientAddress, strlen(recipientAddress));
    tranPtr->amount = amount;
    if (type == 0)
        tranPtr->type = send;
    else tranPtr->type = receive ;};


/*
 * Load transaction data  from a file and stores it in memory.
 * Load transactions based on the inputs which relate to the transaction type.
 * tranPtr and filenames will both contain variables relating to either the pending or confirmed file types.
 */
void loadTransactions (Transaction *tranPtr[], char * filenames[], char *path, int numberOfFiles)
{
    int i, z;
    char fullPath[100], strTempData[500], *tokenArray[MAX_LEN];
    FILE *demodata_file;

    for (i = 0; i < numberOfFiles; i++)
    {
        sprintf(fullPath, "%s%s%s", path, filenames[i], "MCW.txt");
        demodata_file = fopen(fullPath, "r");

        if (demodata_file == NULL)
        {
            printf("Error! No transaction found for file!\n");
            continue;
        }
        else
        {
            fgets(strTempData, 256, demodata_file);
            tranPtr[i] = malloc(sizeof(Transaction));

            tokeniseStringToArray (tokenArray, strTempData);

            initTransaction(tranPtr[i], atoi(filenames[i]), tokenArray[0], strtof(tokenArray[1], NULL), atoi(tokenArray[2]));
            // Free the token array memory
            for (z = 0; z < 1; z++) {
                free(tokenArray[z]);
            }
            fclose(demodata_file);
        }
    }
};

/*
 * Main function to load transactional data from the respective file.
 */
void load_data(char ** confirmedFileList, int confirmedCount, char **pendingFileList, int pendingCount){

    char *paths[] = { "TransactionsPending/", "TransactionsConfirmed/" };
    // Because we now have a list of the pending and confirmed filenames straight from their respective folder
    // we no longer need to hardcode the filenames here.

    // Update the global variables here for size.
    // This is a bit of legacy code from the previous Part A,
    // it would probably be better to steer away from globals if possible.
    pendingSize = pendingCount;
    confirmedSize = confirmedCount;
    // Load transactions by transaction type
    loadTransactions(pendingTransPtr, pendingFileList, paths[0], pendingSize);
    loadTransactions(confirmedTransPtr, confirmedFileList, paths[1], confirmedSize);
};

void viewTransactions(){

    char choice[1], choice2[1], pickedChoice[1];
    puts("1. Confirmed. \n2. Unconfirmed");
    puts("\nPlease select confirmed or pending (unconfirmed) transactions.");
    fgets(choice, 4, stdin);

    validInput(choice, 50);
    pickedChoice[0] =  choice[0];

    puts("\n1. All. \n2. Sent \n3. Received");
    puts("\nPlease select a Transaction Type: ");
    fgets(choice2, 8, stdin);
    validInput(choice2, 51);
    // Print out the transaction based on the transaction type and direction.
    if (pickedChoice[0] == 49)
        printTransactions(confirmedTransPtr, status[0], type[choice2[0] - 49], confirmedSize);
    else printTransactions(pendingTransPtr, status[1], type[choice2[0] - 49] , pendingSize);
};

void printTransactions (Transaction *ptr[],  char * status, char * transactionType, int numberOfFiles){

    printf("\nTransaction Status: %s\n", status);
    printf("\nTransaction Type: %s\n", transactionType);
    int j;
    for (j = 0; j < numberOfFiles; j++){
        // Print out the requested type, either sent, recieved or all.
        if (strcmp(type[ptr[j]->type + 1], transactionType) == 0 || strcmp (transactionType, "All") == 0){
            printf("\nFile Name: %ld", ptr[j]->fileName);
            printf("\nAddress: %s", ptr[j]->recipientAddress);
            printf("\nAmount: %.02lf", ptr[j]->amount);
            printf("\nType: %d\n", ptr[j]->type);}
    }
};

/*
 * Calculates totals for confirmed and pending transaction types.
 * Prints these to the console.
 *
 */
void calculateTotals (){

    static float *resultBuffer;
// Calculate the totals and store in a buffer
    resultBuffer = calculateTotalPerStatus(pendingTransPtr, pendingSize, status[1]);
    // Print to the console.
    // Separating this functionality allows reuse of the calculateTotalPerStatus without printing to the screen.
    printTotals(status[1], resultBuffer);

    free(resultBuffer);

    resultBuffer = calculateTotalPerStatus(confirmedTransPtr, confirmedSize, status[0]);
    printTotals(status[0], resultBuffer);

    free(resultBuffer);
};

/*
 * This calculates totals per the inputted status.
 *
 */
float * calculateTotalPerStatus (Transaction *ptr[], int size , char * status){

    int j;
    float total = 0, send = 0, receive = 0;
    float *resultBuffer = malloc(sizeof(float) * 3);
    for (j = 0; j < size; j++){
        total = total + ptr[j]->amount;
        if (ptr[j]->type == 0)
            send = send + ptr[j]->amount;
        else
            receive = receive + ptr[j]->amount;}
    resultBuffer[0] = total;
    resultBuffer[1] = send;
    resultBuffer[2] = receive;

    if (strcmp(status, "pending") == 0)
        pendingBalance = receive - send;
    else confirmedBalance = receive - send;

    return resultBuffer;
};

/*
 * Prints information about the User's M-Coin situation.
 * Calculates and prints to the screen.
 */
void printTotals (char * status, float resultBuffer[]){
    printf("Total Amount of %s transactions: %.02f\n", status, resultBuffer[0]);
    printf("Total Sent Amount of %s transactions: %.02f\n", status, resultBuffer[1]);
    printf("Total Received Amount of %s transactions: %.02f\n", status,resultBuffer[2]);
    if (strcmp(status, "confirmed") == 0)
        printf("\nThe available balance held by the user: %.02f\n", confirmedBalance - pendingBalance);
}

void viewCoinBalance(){

    // Load the balance if not already loaded.
    if (confirmedBalance == 0){
        calculateTotalPerStatus(confirmedTransPtr, confirmedSize, status[0]);}

    if (pendingBalance == 0){
        calculateTotalPerStatus(pendingTransPtr, pendingSize, status[1]);}

    int j = 0;
    int second = 0;
    int first = 0;
    first = (int) confirmedTransPtr[0]->fileName;
    // Find the latest transaction
    for (j = 1; j < confirmedSize; j++){
        second = (int) confirmedTransPtr[j]->fileName;
        if (second > first)
            first = second;}

    printf("Your public address: 0x%s", USER_ADDRESS);
    printf("\nTotal Amount MCT Coin held by user: %.02f ", confirmedBalance);
    printf("\nThe available balance held by the user: %.02f ", confirmedBalance - pendingBalance);
    printf("\nThe most recent confirmed transaction: %d", first);
    printf("\nYour MCT coin equivalent in euro: %.02f", confirmedBalance * .26);
    puts("\n\nThe total sent and received MCT coin to date:");
    calculateTotals();
}

/*
 * Initialises the recipient data type.
 */
void initRecipient(Recipient *rec, char address[], char label[]){
    strcpy(rec->address, address);
    if (strcmp(label, "-1") == 0)
        strlcpy(rec->label, "No label provided", 18);
    else strcpy(rec->label, label);
};

/*
 * Adds a new recipient and saves to the storedAddress file.
 */
void addNewRecipient(){

    char address[100], addressLabel[200];
    char label[100], label2[100];
    int  i, j, cmp, length, errorSentinal = 1;

    puts("Please enter the 40 digit hex address of the new recipient.");

    // Check the input is valid.
    while (errorSentinal == 1) {
        errorSentinal = 0;
        fgets(address, (char) sizeof(address) + 1, stdin);
        address[strcspn(address, "\n")] = 0;
        length = strlen(address);
        if (strcmp(address, USER_ADDRESS) == 0)
        {
            puts("You cannot send M-Coin to yourself! Enter another address.");
            errorSentinal = 1;
        }
        else if (length != 40){
            puts("Address length is not correct! Must 40 character (20 Hex Bytes). Please try again.");
            errorSentinal = 1;
        }
        for (i = 0; i < length; i++) {
            if (address[i] != 10 && address[i] != 0){
                if (!isxdigit(address[i]))
                {
                    puts("Not a valid address. Please try again.");
                    errorSentinal = 1;
                }
            }
        }
    }

    puts("Please enter an optional one word label for the recipient or -1 to disregard.");

    fgets(label, (char) sizeof(label), stdin);
    strcpy(label2, label);
    label2[strcspn(label2, "\n")] = '\0';

    cmp = strcmp(label2, "-1");
// Ignore the validation if the user enter -1
    if (cmp != 0)
        for (j = 0; j <= strlen(label); j++) {
            if (label[j] != 10 && label[j] != 0){
                if (!isalpha(label[j]) || !isnumber(label[j]) && isspace(label[j]))
                {
                    puts("Not a valid label.");
                    fgets(label, 8, stdin);
                }
            }
        }

    // Load recipients if it hasn't already been done.
    if (noOfRecipients == 0){
        loadRecipients();}
    // Initialise and save the new recipient.
    recipients[noOfRecipients] = malloc(sizeof(Recipient));
    initRecipient(recipients[noOfRecipients], address, label);

    FILE *address_file = NULL;

    char *addressFileName = "StoredAddress/AddressFile.txt";

    address_file = fopen(addressFileName, "a");

    if (address_file == NULL)
        printf("Error! Could not open address file!\n");

    sprintf(addressLabel, "\n%s%s%s", address, " ", label2);
    fprintf(address_file, "%s", addressLabel);
    fclose(address_file);
    puts ("\nRecipient added");
};

/*
 * Loads any recipients that are stored in the storedAddress file.
 *
 */
void loadRecipients(){
    int z, noOfLines = 0;
    char *tokenArray[MAX_LEN], strTempData[MAX_LEN], *addressFileName = "StoredAddress/AddressFile.txt";
    noOfRecipients = 0;

    FILE *address_file = NULL;
    address_file = fopen(addressFileName, "r");

    if (address_file == NULL)
        printf("Error!Could not open address file!\n");

    else
    {
        // Read the strings from the file
        while (fgets(strTempData, MAX_LEN, address_file) != NULL) {
            // Remove the trailing newline character
            if (strchr(strTempData, '\n'))
                strTempData[strlen(strTempData) - 1] = '\0';
            recipients[noOfLines] = malloc(sizeof(Recipient));
            // Split the string data to an array
            tokeniseStringToArray (tokenArray, strTempData);
            // Create a new recipient to store in memory
            initRecipient(recipients[noOfLines], tokenArray[0], tokenArray[1]);

            for (z = 0; z < 1; z++) {
                free(tokenArray[z]);
            }
            noOfLines++;
        }
        fclose(address_file);
    }
    noOfRecipients = noOfLines;
};

/*
 * Splits a string into individual words and stored these in an array
 *
 */
void tokeniseStringToArray(char *tokenArray[], char strTempData[]){
    char *token = strtok(strTempData, " ");
    int j = 0;
    while (token != NULL)
    {
        tokenArray[j] = malloc(strlen(token) + 2);
        strcpy(tokenArray[j], token);
        token = strtok(NULL, " ");
        j++;
    }
};

/*
 * Sends coin to the inputted address and for the request amount.
 * Adds a transaction to the pending queue and saves the transaction to the pending file.
 *
 */
void sendCoin(){
    int i, errorSentinal = 0;
    char agree[3], timestampName[100], addressStr[60], inputAddressStr[50], amountStr[50];
    float amount = 0, availableFunds = 0;
    char* pend;

    // Again get the balance if it hasn't already been done
    if (confirmedBalance == 0){
        calculateTotalPerStatus(confirmedTransPtr, confirmedSize, status[0]);}

    if (pendingBalance == 0){
        calculateTotalPerStatus(pendingTransPtr, pendingSize, status[1]);}

    availableFunds = confirmedBalance - pendingBalance;

    printf("\nConfirmed Balance: %.02f\nPending Balance: %.02f\nAvailable funds: %.02f\n", confirmedBalance, pendingBalance, availableFunds);

    loadRecipients();

    printf("\nRecipients No. %d\n", noOfRecipients);

    for (i = 0; i < noOfRecipients; ++i) {
        printf("\nAddress No. %d : %s \nLabel: %s", i + 1, recipients[i]->address, recipients[i]->label);
    }

    // More validation of input
    while (errorSentinal == 0) {
        errorSentinal = 1;
        puts("\n\nInput address No. and amount of MCT coin to send.");
        puts("\nAddress No. : ");

        fgets(inputAddressStr, 8, stdin);
        validInput(inputAddressStr, noOfRecipients + 49);

        if (recipients[inputAddressStr[0] - 49]->address == NULL){
            puts("\n\nThe selected address does not exist. Please try again.");
            errorSentinal = 0;}

        else {
            puts("\nAmount : ");
            fgets(amountStr, 8, stdin);
            amount = strtof(amountStr, &pend);

            if (amount > availableFunds){
                printf("\nThe amount selected, %.02f, is greater than you available balance, %.02f, when taking pending transactions into account ! Please try a smaller amount..",amount, availableFunds);
                errorSentinal = 0;}

            else printf("\nYou've requested to send %.02f to the address : %s, labelled: %s\nEnter '1' to confirm, or anything else to go back to menu",
                        amount, recipients[inputAddressStr[0] - 49]->address, recipients[inputAddressStr[0] - 49]->label);

            fgets(agree, 8, stdin);
            agree[strcspn(agree, "\n")] = '\0';

            if (strcmp(agree, "1") == 0){
                pendingTransPtr[pendingSize] = malloc(sizeof(Transaction));
                time_t now = (int)time(NULL);

                // Initialise and save the transaction
                initTransaction(pendingTransPtr[pendingSize], now, recipients[inputAddressStr[0] - 49]->address, amount, send);
                pendingSize++;

                sprintf(timestampName, "%s%d%s", "TransactionsPending/", now, "MCW.txt");

                FILE *newPender = NULL;
                newPender = fopen(timestampName, "w");

                sprintf(addressStr, "%s %.04f%s", recipients[inputAddressStr[0] - 49]->address, amount, " 0");
                fwrite(addressStr , 1 , 50 , newPender);

                puts ("\nTransaction has been added to the pending queue");
                fclose(newPender);
            }
        }
    }
};

/*
 * Sort either the pending or confirmed transactions by address or amount.
 *
 */
int sortTransactions() {

    char type[1], sortBy[1], concreteChoice[1];
    Transaction *confirmed[confirmedSize];
    Transaction *pending[pendingSize];

    puts("Enter the transaction type to sort.\n1. Confirmed \n2. Pending");
    fgets(type, 8, stdin);
    validInput(type, 50);
    concreteChoice[0] = type[0];

    puts("Enter the parameter to sort by.\n1. Recipient Address \n2. Amount");
    fgets(sortBy, 8, stdin);
    validInput(sortBy, 50);
    if (concreteChoice[0] == 49)
        sortByTransactionType(confirmed, confirmedTransPtr, confirmedSize, sortBy[0] - 48, status[0]);
    else
        sortByTransactionType(pending, pendingTransPtr, pendingSize, sortBy[0] - 48, status[1]);
};

/*
 * Helper function to sort by individual type.
 *
 */
void sortByTransactionType(Transaction *sortingArray[], Transaction *sourceArray[],  int arraySize, int sortBy, char * status){
    int i;
    for (i = 0; i < arraySize; ++i) {
        sortingArray[i] = malloc(sizeof(Transaction));
        sortingArray[i] = sourceArray[i];}
    bubbleSortTransactions(sortingArray, arraySize, sortBy);
    printTransactions(sortingArray, status, "All", arraySize);
};

/*
 * A common bubble sort algorithm to sort the transactions.
 *
 */
void bubbleSortTransactions(Transaction *array[], int arraySize, int sortBy) {
    int i = 0,   // i = outer index
            j = 0;   // j = inner index
    Transaction *swap;

    if (sortBy == 1)
        // Sort by address
        for (i = 0; i < (arraySize - 1); ++i) {
            for (j = 0; j < (arraySize - i - 1); ++j) {
                if (strcmp(array[j]->recipientAddress, array[j + 1]->recipientAddress) >= 0) {
                    swap = array[j];
                    array[j] = array[j + 1];
                    array[j + 1] = swap;
                }
            }
        }
    else
        // Sort by amount
        for (i = 0; i < (arraySize - 1); ++i) {
            for (j = 0; j < (arraySize - i - 1); ++j) {
                if (array[j]->amount >= array[j + 1]->amount) {
                    swap = array[j];
                    array[j] = array[j + 1];
                    array[j + 1] = swap;
                }
            }
        }

};

/*
 * Exit the application, freeing any memory as it does.
 *
 */
void exitMCW(){
    int i;
    storePendingTransactionInfo();
    puts("\nExiting the M-Coin Wallet App!");
    // Free any transactions or recipients that may exist
    for (i = 0; i < confirmedSize; ++i) {
        free(confirmedTransPtr[i]);}
    for (i = 0; i < pendingSize; ++i) {
        free(pendingTransPtr[i]);}
    for (i = 0; i < noOfRecipients; ++i) {
        free(recipients[i]);}
};


/*
 * Read all the files in the pending directory and saves their filenames to a file
 */

void storePendingTransactionInfo(){

    int i;
    FILE * writeFileNames = NULL;
    writeFileNames = fopen("TransactionsPending/pendingTransactions.txt", "w");
    for (i = 0; i < pendingSize; ++i) {
        fprintf(writeFileNames, "%ld\n", pendingTransPtr[i]->fileName);}
    fclose(writeFileNames);
};

/*
 * Reads the names of the pendingTransactions.txt file and stores the filenames in memory.
 */
int readPendingTransactionInfo(){
    int noOfLines = 0;
    char *tokenArray[MAX_LEN], strTempData[MAX_LEN];

    FILE *pendingFileNames = NULL;
    pendingFileNames = fopen("TransactionsPending/pendingTransactions.txt", "r");

    if (pendingFileNames == NULL)
        printf("Error!Could not open address file!\n");

    else
    {
        while (fgets(strTempData, MAX_LEN, pendingFileNames) != NULL) {
            // Remove the trailing newline character
            if (strchr(strTempData, '\n'))
                strTempData[strlen(strTempData) - 1] = '\0';
            pendingBuffer[noOfLines] = (char*)malloc(sizeof(char) * 10);
            tokeniseStringToArray (tokenArray, strTempData);
            pendingBuffer[noOfLines] =  tokenArray[0];
            free(tokenArray[0]);
            noOfLines++;
        }
        fclose(pendingFileNames);
    }

    return noOfLines;
};

/*
 * Checks if any files in the that were in the pending directory are now in the confirmed directory.
 * Returns a list of any file names that have transitioned from pending to confirmed.
 */
char ** checkNotifications(int pendingFromLastSession, char ** validFileList, int count, int * noOfNotifications){

    int i = 0,j = 0, noOfNotify = 0, z = 0; //Number of valid files
    char **transitionedFileList; //Pointer to memory buffer where the list of valid confirmation files will be kept.
    transitionedFileList = (char**)malloc(sizeof(char*)* GROWBY);

    for (j = 0; j < count; j++){
        for (i = 0; i < pendingFromLastSession; i++){
            if (strcmp(validFileList[j], pendingBuffer[i]) == 0)
            {
                transitionedFileList[noOfNotify] = malloc(sizeof(char)* VALID_TIMESTAMP_LENGTH + 1);
                transitionedFileList[noOfNotify] = pendingBuffer[i];
                noOfNotify++;
                free(pendingBuffer[j]);
            }
        }
    }
    *noOfNotifications = noOfNotify;
    return transitionedFileList;
};

/* Function Name:	 find_valid_transaction_files
*  Paramaters:	  int * confirmed_transactions_count - number of confirmed transactions
*
*  const char * filename - name of file to be validated
*				  char * pszValidtimestamp - address of valid timestamp data to be updated if found
*
*  Returns:		char** - pointer to block of memory that contains the list of confirmed transactions.
*
*  Purpose:
*  Search directory for all valid and confirmed transaction files
*  Stores the timestamp component of each file in a memory block
*
*  Prequisites/Requirements:
*  Installation of the Dirent api
*
*  Usage Directions:
*  1. Install Dirent API
*  2. Copy and paste this code to your project source file
*  2. Call Function find_valid_transaction_files as in the main() example above
*
*
**************************************************************************************************/
char ** find_valid_transaction_files(int * confirmed_transactions_count, int fileType)
{
    //char *transactions_conf[1000];
    char currentfile[500];
    char validtimestamp[VALID_TIMESTAMP_LENGTH + 1];
    int lineno = 0;
    const char *paths[MAX_LEN] = {TRANSACTIONS_CONFRIMED_PATH, TRANSACTIONS_PENDING_PATH};
    char **file_timestamps; //pointer to an array of file timestamps
    /*
    * Let's make some space  in memory for each of the Pointers that will point
    * to the valid file_timestamps we have obtained.
    *
    * We request space to hold a  block of <addresses> (pointers to characters - not "char" but "char *")
    *
    * GROWBY (4096) Pointers for all the Lines should be enough!
    *
    */
    file_timestamps = (char**)malloc(sizeof(char*)* GROWBY);



    //This code uses the dirent api to traverse the directory
    //You will need to install it so that it works.
    //
    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir(paths[fileType])) != NULL)
    {
        /* Get all the files and directories within TRANSACTIONS_CONFRIMED_PATH */
        while ((ent = readdir(dir)) != NULL)
        {
            strcpy(currentfile, ent->d_name);
            //printf("Current file is %s\n", currentfile);
            if (check_valid_filename(currentfile, validtimestamp) == VALID)
            {

                //printf("Confirmed Transaction at timestamp is %s\n", validtimestamp);
                file_timestamps[lineno] = malloc(sizeof(char)* VALID_TIMESTAMP_LENGTH + 1);

                if (file_timestamps[lineno] == NULL)
                {
                    printf("Error allocating memory");
                    getchar();
                    exit(EXIT_FAILURE);
                }
                else
                {
                    //Copy the data
                    //printf("Allocated Memory!\n");
                    strcpy(file_timestamps[lineno], validtimestamp);
                    lineno++;
                }

                /*
                * Every time we reach a multiple of GROWBY we increase the
                *
                * size of the array by GROWBY, so we can handle
                *
                * arbitrary sized files, only bounded by our memory size
                *
                */

                if ((lineno % GROWBY) == 0)

                {
                    /*
                    *If we have done GROWBY lines then we need more memory!
                    *
                    */
                    file_timestamps = (char**)realloc(file_timestamps, sizeof(char*)* (GROWBY + lineno));
                    //printf("Reallocated more memory when at line %d\n", lineno);
                    getchar();
                }

            }


        }
        closedir(dir);
        *confirmed_transactions_count = lineno;
        return file_timestamps;
    }
    else {
        /* could not open directory */
        perror("could not open directory");
        exit(EXIT_FAILURE);
    }
}




/* Function Name: check_valid_filename
*  Paramaters:	  const char * filename - name of file to be validated
*				  char * pszValidtimestamp - address of valid timestamp data to be updated if found
*
*  Returns:		Enum FileType - value indicates validity of filename
*
*  Purpose:
*  Given a filename checks it's a valid confirmation transaction.
*  If yes updates pszValidtimestamp with the timestamp component of the file.
*
*
**************************************************************************************************/
FileType check_valid_filename(const char *filename, char *pszValidtimestamp)
{
    FileType valid_filename = INVALID_UNKNOWN;
    int filename_length;
    time_t timestamp_t;
    time_t now;

    char *end_location;
    char timestamp[VALID_TIMESTAMP_LENGTH+1];
    const char *dot = strrchr(filename, '.');


    //Simple invalid case
    if (!dot || dot == filename) {
        valid_filename = INVALID_DOT;
        return valid_filename;
    }

        //
        //Check Filename length is that of timestamp file
        // Sample: 1519811094MCW.txt
        // 17 characters or VALID_CONFIRMATION_FILE_LENGTH
    else
    {
        filename_length = strlen(filename);
        //printf("filename_length is %d\n", filename_length);
        if (filename_length != VALID_CONFIRMATION_FILE_LENGTH)
        {
            valid_filename = INVALID_LENGTH;
            return valid_filename;
        }
    }

    //Check for the proper Suffix

    //printf("Looking for %s in %s \n", suffix, filename);
    end_location = strstr(filename, suffix);
    if (end_location == NULL)
    {
        //printf("String not found in %s\n", filename);
        valid_filename = INVALID_SUFFIX;
        return valid_filename;
    }
        //Valid File Suffix found!
        //Extract timestamp,
        //Check it's numeric and not in the future!
        //
    else
    {
        //printf("Found %s in %s \n", suffix, filename);

        //Extract the 10 chars that represent the timestamp
        strncpy(timestamp, filename, VALID_TIMESTAMP_LENGTH);
        timestamp[VALID_TIMESTAMP_LENGTH] = '\0';
        //printf("Timestamp found is %s\n\n", timestamp);

        //Check each char is numeric
        for (int i = 0; i < strlen(timestamp); i++)
        {
            if (isdigit(timestamp[i]))
                continue;
            else
            {
                valid_filename = INVALID_NONNUMERIC;
                return valid_filename; //bad timestamp
            }

        }
        //printf("Timestamp is numeric %s\n\n", timestamp);

        //Check timestamp not in the future!
        //

        timestamp_t = (time_t) atoi(timestamp);
        now = time(NULL) + 10;
        if (timestamp_t > now)
        {
            printf("ERROR: Invalid timestamp %s is in the future: %s\n", timestamp, ctime(&timestamp_t));
            valid_filename = INVALID_FUTURETIME;
            return valid_filename;
        }
        else
        {
//            printf("%s is a confirmed transaction which occured on  %s", timestamp, ctime(&timestamp_t));
            strcpy(pszValidtimestamp, timestamp);
            valid_filename = VALID;
            return valid_filename;
        }

    }

    return valid_filename;
}//End Fucntion




