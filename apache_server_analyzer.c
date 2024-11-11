#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_LINE_LENGTH 2048
#define MAX_RESOURCES 50000
#define MAX_REFERERS 50000
#define MAX_URI_LENGTH 1024

// Structure to store resource/referer counts
typedef struct {
    char uri[MAX_URI_LENGTH];
    int count;
} CountEntry;

// Structure to store analysis results
typedef struct {
    int post_count;
    int get_count;
    CountEntry resources[MAX_RESOURCES];
    int unique_resources;
    CountEntry referers[MAX_REFERERS];
    int unique_referers;
    int status_codes[1000];  // More than enough for all possible HTTP status codes
    int unique_status_codes;
} LogAnalysis;

// Function to find or add a URI in CountEntry array
int find_or_add(CountEntry* entries, int* unique_count, const char* uri) {
    // Search for existing entry
    for (int i = 0; i < *unique_count; i++) {
        if (strcmp(entries[i].uri, uri) == 0) {
            entries[i].count++;
            return i;
        }
    }
    
    // Add new entry
    if (*unique_count < MAX_RESOURCES) {
        strncpy(entries[*unique_count].uri, uri, MAX_URI_LENGTH - 1);
        entries[*unique_count].uri[MAX_URI_LENGTH - 1] = '\0';
        entries[*unique_count].count = 1;
        (*unique_count)++;
    }
    return *unique_count - 1;
}

// Function to add status code
void add_status_code(LogAnalysis* analysis, int status_code) {
    bool found = false;
    for (int i = 0; i < analysis->unique_status_codes; i++) {
        if (analysis->status_codes[i] == status_code) {
            found = true;
            break;
        }
    }
    if (!found) {
        analysis->status_codes[analysis->unique_status_codes++] = status_code;
    }
}

// Function to find most common entry in CountEntry array
CountEntry find_most_common(CountEntry* entries, int count) {
    CountEntry max = entries[0];
    for (int i = 1; i < count; i++) {
        if (entries[i].count > max.count) {
            max = entries[i];
        }
    }
    return max;
}

// Function to count status code occurrences
int count_status_code(FILE* file, int target_code) {
    char line[MAX_LINE_LENGTH];
    int count = 0;
    char method[10];
    char uri[MAX_URI_LENGTH];
    int status;
    
    // Return to start of file
    fseek(file, 0, SEEK_SET);
    
    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "%*[^\"]\"%[^ ] %[^\"]\" %d", method, uri, &status) == 3) {
            if (status == target_code) {
                count++;
            }
        }
    }
    return count;
}

int main() {
    FILE* file = fopen("log.txt", "r");
    if (!file) {
        printf("Error opening file\n");
        return 1;
    }

    LogAnalysis analysis = {0};  // Initialize all counts to 0
    char line[MAX_LINE_LENGTH];
    char method[10];
    char uri[MAX_URI_LENGTH];
    char referer[MAX_URI_LENGTH];
    int status;

    // Process each line
    while (fgets(line, sizeof(line), file)) {
        // Parse the log line
        if (sscanf(line, "%*[^\"]\"%[^ ] %[^\"]\" %d %*d \"%[^\"]\"", 
                   method, uri, &status, referer) == 4) {
            
            // Count request methods
            if (strcmp(method, "POST") == 0) {
                analysis.post_count++;
            } else if (strcmp(method, "GET") == 0) {
                analysis.get_count++;
            }

            // Track unique resources
            find_or_add(analysis.resources, &analysis.unique_resources, uri);

            // Track referers (if not "-")
            if (strcmp(referer, "-") != 0) {
                find_or_add(analysis.referers, &analysis.unique_referers, referer);
            }

            // Track status codes
            add_status_code(&analysis, status);
        }
    }

    // Find the most common status code and count its occurrences
    CountEntry most_common_resource = find_most_common(analysis.resources, analysis.unique_resources);
    CountEntry most_common_referer = find_most_common(analysis.referers, analysis.unique_referers);

    // Print results
    printf("1. POST requests: %d\n", analysis.post_count);
    printf("2. GET requests: %d\n", analysis.get_count);
    printf("3. Unique client requested resources: %d\n", analysis.unique_resources);
    printf("4. Most common requested resource: %s (%d times)\n", 
           most_common_resource.uri, most_common_resource.count);
    printf("5. Different HTTP response status codes: %d\n", analysis.unique_status_codes);
    
    // Find most common status code
    int max_status_count = 0;
    int most_common_status = 0;
    for (int i = 0; i < analysis.unique_status_codes; i++) {
        int current_count = count_status_code(file, analysis.status_codes[i]);
        if (current_count > max_status_count) {
            max_status_count = current_count;
            most_common_status = analysis.status_codes[i];
        }
    }
    
    printf("6. Most common status code (%d) appears: %d times\n", 
           most_common_status, max_status_count);
    printf("7. Most common referer: %s\n", most_common_referer.uri);

    fclose(file);
    return 0;
}