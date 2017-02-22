#include <mpi.h>
#include <netdb.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <ctime>

#define clinicSize 5
#define windowsCount 2
#define MAX_SIZE 256

using namespace std;


// msg[0] = rank;
// msg[1] = clinicCount;
// msg[2] = type;

void printQueue(int rank, vector <pair <int, int>> queue){
    for(int i=0; i<queue.size(); i++) {
        printf("%d: process[%d]: %d, %ld\n", rank, i, queue[i].first, queue[i].second);
    }
}

void addToQueueSorted(int rank, int priority, vector <pair <int, int>> &queue) {
    pair<int, int> process;
    process.first = rank;
    process.second = priority;
    if (queue.size() == 0) {
        queue.push_back(process);
    }
    else {
        for (int i = 0; i < queue.size(); i++) {
            if ((process.second < queue[i].second) || (process.second == queue[i].second && process.first < queue[i].first)) {
                queue.insert(queue.begin()+i, process);
                return;
            }
        }
        queue.push_back(process);
    }
}

void receiveMessages(int size, int rank, int *msg, vector <pair <int, int>> &queue, int tag){
    MPI_Status status;
    int i = 0;
    while(i != size - 1) {//good
        MPI_Recv(msg, 3, MPI_LONG_INT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status);
        if (msg[2] == 1) { //kolejka do kliniki
            //printf("%d: otrzymalem wiadomosc od %d, proces chce wejsc do kliniki\n", rank, msg[0]);
            addToQueueSorted(msg[0], msg[1], queue);
        }
        else if (msg[2] == 2) {//kolejka do okienka
            //printf("%d: otrzymalem wiadomosc od %d, proces chce sie dostac do okienka\n", rank, msg[0]);
            addToQueueSorted(msg[0], msg[1], queue);
        }
        else if (msg[2] == 3) {//zwolnienie okienka
            //printf("%d: proces %d mowi, ze zwolnilo sie miejsce przy okienku\n", rank, msg[0]);
        }
        else if (msg[2] == 4) {//zwolnienie kliniki
            //printf("%d: proces %d mowi, ze zwolnilo sie miejsce w klinice\n", rank, msg[0]);
        }
        else if (msg[2] == 5) {//zwolnienie kliniki
            //printf("%d: proces %d mowi, ze ma %d idiotow\n", rank, msg[0], msg[1]);
            addToQueueSorted(msg[0], msg[1], queue);
        }
        i++;
    }
}

int getPosition(int rank, vector <pair <int, int>> queue){
    for(int i = 0; i < queue.size() ; i++){
        if(rank == queue[i].first )
            return i;
        }
    return -1;
}

void eraseQueue(vector <pair <int, int>> &queue)
{   
    while(queue.size() != 0) {
        queue.pop_back();
        queue.back();
    }
}

void updateWindowsQueueCompanies(vector <pair <int, int>> queue, vector <int> &companies, int count) {
    for (int i = 0; i < count; i++) {
        companies.push_back(queue[i].first);
    }
}

void pop_front(vector<int> &queue, int count) {
    reverse(queue.begin(), queue.end());
    for (int i=0; i<count; i++) {
        if (!queue.empty()) {
            queue.pop_back();
            queue.back();
        }
    }
    reverse(queue.begin(), queue.end());
}

int getIdiotsBeforeMe(vector <pair <int, int>> clinicQueue, vector <pair <int, int>> idiotsPerCompany, int rank) {

    int idiotsBefore = 0;
    for (int i=0; i<clinicQueue.size(); i++) {
        for (int j=0; j<idiotsPerCompany.size(); j++) {
            if (clinicQueue[i].first == rank) {
                return idiotsBefore;
            }
            else if (clinicQueue[i].first == idiotsPerCompany[j].first) {
                idiotsBefore += idiotsPerCompany[j].second;
                break;
            }
        }
    }

    return 0;
}

int getCompaniesInClinicCount(vector <pair <int, int>> clinicQueue, vector <pair <int, int>> idiotsPerCompany) {

    int companiesIn = 0;
    int spaceLeft = clinicSize;
    for (int i=0; i<clinicQueue.size(); i++) {
        for (int j=0; j<idiotsPerCompany.size(); j++) {
            if (clinicQueue[i].first == idiotsPerCompany[j].first) {
                if(spaceLeft > 0) {
                    companiesIn++;
                    spaceLeft -= idiotsPerCompany[j].second;
                }
                break;
            }
        }
    }

    return companiesIn;
}

int main(int argc, char **argv) {

    vector <pair <int, int>> clinicQueue;
    vector <pair <int, int>> windowsQueue;
    vector <pair <int, int>> idiotsPerCompany;
    vector <int> companiesInWindowsQueue;
    
    int size, rank, len, clinicCount = 0, windowsQueueCount = 0, loop = 1, idiotsCount = 0;
    char processor[24];
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // ktory watek
    MPI_Comm_size(MPI_COMM_WORLD, &size); // ile watkow
    MPI_Get_processor_name(processor, &len);
    printf("Jestem: %d z %d na (%s)\n", rank, size, processor);
    int msg[3];
    srand(time(NULL));
    while (loop < 6){
        sleep(rand()%4);//"co pewien czas"
        if (idiotsCount == 0) {
            idiotsCount = rand()%4 +1;//1 - 5 idiotow
        }
        else {
            idiotsCount += rand()%2 + 1;//1 - 3 idiotow
        }
        printf("%d: mam %d idiotow\n", rank, idiotsCount);
        for (int i = 0; i < size; i++) {
            if (i != rank) {
                msg[0] = rank;
                msg[1] = clinicCount;
                msg[2] = 1;//kolejka do kliniki
                MPI_Send(msg, 3, MPI_LONG_INT, i, msg[2], MPI_COMM_WORLD);
                //printf("%d: wysylam wiadomosc do %d - chce wejsc do kliniki\n", rank, i);
                msg[1] = idiotsCount;
                msg[2] = 5;//ilu mam idiotow
                MPI_Send(msg, 3, MPI_LONG_INT, i, msg[2], MPI_COMM_WORLD);
                ///printf("%d: wysylam wiadomosc do %d - mam %d idiotow\n", rank, i, idiotsCount);
            }
        }
        addToQueueSorted(rank, clinicCount, clinicQueue);
        addToQueueSorted(rank, idiotsCount, idiotsPerCompany);
        receiveMessages(size, rank, msg, clinicQueue, 1);//kolejka do kliniki
        receiveMessages(size, rank, msg, idiotsPerCompany, 5);//ile kto ma idiotiow
        //printf("%d: kto ma ilu idiotow:\n", rank);
        //printQueue(rank, idiotsPerCompany);
        int idiotsICanCastrate = max(0,clinicSize - getIdiotsBeforeMe(clinicQueue, idiotsPerCompany, rank));
        printf("%d: moge wykastrowac %d idiotow i wykastruje %d z nich\n", rank, idiotsICanCastrate, min(idiotsICanCastrate, idiotsCount));
        //printf("%d: moja kolejka do kliniki:\n", rank);
        //printQueue(rank, clinicQueue);
        printf("%d: jestem %d w kolejce do kliniki\n", rank, getPosition(rank, clinicQueue));
        int count = getCompaniesInClinicCount(clinicQueue, idiotsPerCompany);//ile firm weszlo do kliniki
        printf("%d: do kliniki wejdzie %d firm\n", rank, count);

        if(idiotsICanCastrate > 0) {
            printf("%d: wchodze do kliniki\n", rank);
            sleep(rand()%2+1 * min(idiotsICanCastrate, idiotsCount));//kastrowanie n idiotow, moga byc oporni...
            printf("%d: skonczylem kastracje\n", rank);
            idiotsCount = max(0, idiotsCount - idiotsICanCastrate);//pozostali idioci
            for (int i=count; i<clinicQueue.size(); i++) {
                msg[0] = rank;
                msg[1] = 0;
                msg[2] = 4;//zwolnienie kliniki
                //printf("%d: wysylam wiadomosc do %d - zwalniam klinike\n", rank, clinicQueue[i].first);
                MPI_Send(msg, 3, MPI_LONG_INT, clinicQueue[i].first, msg[2], MPI_COMM_WORLD);
            }

            clinicCount++;//bylem w klinice
            updateWindowsQueueCompanies(clinicQueue, companiesInWindowsQueue, count);//ile weszlo procesow
            bool isInQueue = true;
            while( isInQueue && companiesInWindowsQueue.size() > 0) {
                for (int i = 0; i < companiesInWindowsQueue.size() ; i++) {
                    if (companiesInWindowsQueue[i] != rank) {
                        msg[0] = rank;
                        msg[1] = windowsQueueCount;
                        msg[2] = 2;//kolejka do okienek
                        //printf("%d: wysylam wiadomosc do %d - chce wejsc do okienka\n", rank, companiesInWindowsQueue[i]);
                        MPI_Send(msg, 3, MPI_LONG_INT, companiesInWindowsQueue[i], msg[2], MPI_COMM_WORLD);//On2
                    }
                }
                addToQueueSorted(rank, windowsQueueCount, windowsQueue);
                receiveMessages(companiesInWindowsQueue.size(), rank, msg, windowsQueue, 2);//kolejka do okienka
                printf("%d: moja kolejka do okienka:\n", rank);
                printQueue(rank, windowsQueue);
                pop_front(companiesInWindowsQueue, windowsCount);
                if (getPosition(rank, windowsQueue) < windowsCount) {
                    printf("%d: podchodze do okienka\n", rank);
                    sleep(rand()%4);//obsluga w okienku - moze pojsc gladko
                    printf("%d: odchodze od okienka\n", rank);
                    isInQueue = false;
                    windowsQueueCount++; 
                    for (int i = 0; i < companiesInWindowsQueue.size() ; i++) {
                        msg[0] = rank;
                        msg[1] = 0;
                        msg[2] = 3;//zwolnienie okienka
                        //printf("%d: wysylam wiadomosc do %d - zwalniam okienko\n", rank, companiesInWindowsQueue[i]);
                        MPI_Send(msg, 3, MPI_LONG_INT, companiesInWindowsQueue[i], msg[2], MPI_COMM_WORLD);
                    }   
                }
                else {
                    printf("%d: nie zmiescilem sie w kolejce do okienka...\n", rank);
                    receiveMessages(windowsCount+1, rank, msg, windowsQueue, 3);//czekam na info zwolnienia okienek
                }
                eraseQueue(windowsQueue);               
            }
            while(companiesInWindowsQueue.size() != 0) {//szybki erase
                companiesInWindowsQueue.pop_back();
                companiesInWindowsQueue.back();
            }           
        }
        else {
            printf("%d: nie zmiescilem sie w kolejce do kliniki...\n", rank);
            receiveMessages(count+1, rank, msg, clinicQueue, 4);//info o zwolnieniu miejsca w klinice
        }
        eraseQueue(clinicQueue);
        eraseQueue(idiotsPerCompany);
        loop++;
    }

    MPI_Finalize();
}