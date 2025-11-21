#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <curl/curl.h>
#include <mutex>
#include <sstream>

using namespace std;

// ------------------------- Global Metrics -------------------------
atomic<int> totalRequests(0);
atomic<long long> totalResponseTimeMs(0);

// ------------------------- CURL Helper ----------------------------
size_t writeCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    return size * nmemb; // ignore response body
}

long makeRequest(const string &url, const string &method, const string &body = "")
{
    CURL *curl = curl_easy_init();
    if (!curl)
        return -1;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 2000L);

    if (method == "POST")
    {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    }

    long response_code = 0;
    auto start = chrono::high_resolution_clock::now();
    CURLcode res = curl_easy_perform(curl);
    auto end = chrono::high_resolution_clock::now();

    long durationMs = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    totalRequests++;
    totalResponseTimeMs += durationMs;

    curl_easy_cleanup(curl);
    return response_code;
}

// ------------------------- CPU Bound Worker --------------------
void ioBoundWorker(int clientId, int maxClientId, int durationSec)
{
    const string serverUrl = "http://localhost:8080"; // hard-coded
    auto endTime = chrono::high_resolution_clock::now() + chrono::seconds(durationSec);
    cout << "hi" << endl;
    while (chrono::high_resolution_clock::now() < endTime)
    {

        // Random receiver between 1..N (excluding itself)
        int receiver = rand() % maxClientId + 1;
        if (receiver == clientId)
            receiver = (receiver % maxClientId) + 1;
        cout<<"hi"<<endl;
        // Send message
        stringstream ss;
        ss << "{\"sender_id\":" << clientId
           << ",\"receiver_id\":" << receiver
           << ",\"msg\":\"Hello from client " << clientId << "\"}";
        cout << "{\"sender_id\":" << clientId
           << ",\"receiver_id\":" << receiver
           << ",\"msg\":\"Hello from client " << clientId <<endl;

        makeRequest(serverUrl + "/message", "POST", ss.str());

        // Fetch messages for this client
        //makeRequest(serverUrl + "/message/" + to_string(clientId), "GET");
    }
}

// ------------------------- IO Bound Worker --------------------
void cpuBoundWorker(int clientId, int maxClientId, int durationSec)
{
    const string serverUrl = "http://localhost:8080"; // hard-coded
    auto endTime = chrono::high_resolution_clock::now() + chrono::seconds(durationSec);

    while (chrono::high_resolution_clock::now() < endTime)
    {
        // Fetch history for THIS client (not random)
        //makeRequest(serverUrl + "/history/" + to_string(clientId), "GET");
        makeRequest(serverUrl + "/recent/" + to_string(clientId), "GET");
    }
}

// ------------------------- Main Function -------------------------
int main(int argc, char **argv)
{
    if (argc < 4)
    {
        cout << "Usage: " << argv[0] << " <num_threads> <duration_sec> <cpu|io>\n";
        return 1;
    }

    int numThreads = stoi(argv[1]);
    int durationSec = stoi(argv[2]);
    string mode = argv[3];

    curl_global_init(CURL_GLOBAL_ALL);

    vector<thread> threads;

    for (int clientId = 1; clientId <= numThreads; clientId++)
    {
        if (mode == "cpu")
            threads.emplace_back(cpuBoundWorker, clientId, numThreads, durationSec);
        else if (mode == "io")
            threads.emplace_back(ioBoundWorker, clientId, numThreads, durationSec);
        else
        {
            cerr << "Invalid mode: cpu or io expected\n";
            return 1;
        }
    }

    for (auto &t : threads)
        t.join();

    double avgResp = totalRequests > 0
                         ? (double)totalResponseTimeMs.load() / totalRequests.load()
                         : 0;

    cout << "\n-------- RESULTS --------\n";
    cout << "Total Requests: " << totalRequests.load() << "\n";
    cout << "Avg Response Time: " << avgResp << " ms\n";
    cout << "Throughput: " << (double)totalRequests.load() / durationSec
         << " req/sec\n";

    curl_global_cleanup();
    return 0;
}
