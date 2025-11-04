/*
 * shop_manager.c - Enhanced Version
 * Command-line Management System for a Computer & Accessories Sales Shop
 * 
 * Features:
 *  - Product inventory management
 *  - Customer management
 *  - Sales management
 *  - Comprehensive user management with permissions
 *  - Advanced reporting
 *  - Secure authentication
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define MAX_LINE 1024
#define MAX_FIELD 256
#define MAX_PASSWORD_LEN 64
#define PRODUCTS_FILE "products.csv"
#define CUSTOMERS_FILE "customers.csv"
#define SALES_FILE "sales.csv"
#define USERS_FILE "users.csv"
#define BACKUP_DIR "backups"

/* -------------------- Data Structures -------------------- */
typedef struct {
    int id;
    char name[100];
    char category[50];
    char brand[50];
    float cost_price;
    float sell_price;
    int stock;
    int min_stock_level;
} Product;

typedef struct {
    int id;
    char name[100];
    char phone[30];
    char email[100];
    char address[200];
} Customer;

typedef struct {
    int id;
    int product_id;
    int customer_id;
    int quantity;
    float total_price;
    char date[64];
    char cashier[50];
} Sale;

typedef struct {
    int id;
    char username[50];
    char password_hash[65];
    int can_manage_products;
    int can_manage_customers;
    int can_manage_sales;
    int can_view_reports;
    int can_manage_users;
    int is_active;
} User;

/* -------------------- Utility Helpers -------------------- */
void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void pause_and_wait() {
    printf("\nPress ENTER to continue...");
    clear_input_buffer();
}

void trim_newline(char *str) {
    str[strcspn(str, "\n")] = 0;
}

int get_validated_int(const char *prompt, int min, int max) {
    int value;
    char input[50];
    
    while (1) {
        printf("%s", prompt);
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("Error reading input.\n");
            continue;
        }
        
        if (sscanf(input, "%d", &value) == 1 && value >= min && value <= max) {
            return value;
        }
        printf("Invalid input. Please enter a number between %d and %d.\n", min, max);
    }
}

float get_validated_float(const char *prompt, float min) {
    float value;
    char input[50];
    
    while (1) {
        printf("%s", prompt);
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("Error reading input.\n");
            continue;
        }
        
        if (sscanf(input, "%f", &value) == 1 && value >= min) {
            return value;
        }
        printf("Invalid input. Please enter a number greater than or equal to %.2f.\n", min);
    }
}

void get_validated_string(const char *prompt, char *buffer, size_t size) {
    while (1) {
        printf("%s", prompt);
        if (fgets(buffer, size, stdin) == NULL) {
            printf("Error reading input.\n");
            continue;
        }
        
        trim_newline(buffer);
        if (strlen(buffer) > 0) {
            return;
        }
        printf("Input cannot be empty. Please try again.\n");
    }
}

void now_str(char *buffer, size_t size) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", &tm);
}

int file_exists(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    fclose(f);
    return 1;
}

int create_directory(const char *path) {
    char command[256];
    snprintf(command, sizeof(command), "mkdir -p %s", path);
    return system(command);
}

/* -------------------- CSV Parsing Helpers -------------------- */
char* parse_csv_field(char *line, int field_num) {
    static char field[MAX_FIELD];
    char *start = line;
    int current_field = 0;
    int in_quotes = 0;
    int field_index = 0;
    
    while (*start && current_field <= field_num) {
        if (*start == '"') {
            in_quotes = !in_quotes;
            start++;
            continue;
        }
        
        if (current_field == field_num) {
            if (field_index < MAX_FIELD - 1) {
                if (!in_quotes && *start == ',') {
                    break;
                }
                field[field_index++] = *start;
            }
        }
        
        if (!in_quotes && *start == ',') {
            current_field++;
        }
        
        start++;
    }
    
    field[field_index] = '\0';
    
    // Remove surrounding quotes if present
    if (field_index > 1 && field[0] == '"' && field[field_index-1] == '"') {
        memmove(field, field + 1, field_index - 2);
        field[field_index - 2] = '\0';
    }
    
    return (field_index > 0) ? field : NULL;
}

/* -------------------- Backup System -------------------- */
void create_backup() {
    create_directory(BACKUP_DIR);
    
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char backup_name[100];
    strftime(backup_name, sizeof(backup_name), "%Y%m%d_%H%M%S", &tm);
    
    char command[512];
    snprintf(command, sizeof(command), "cp *.csv %s/backup_%s 2>/dev/null", BACKUP_DIR, backup_name);
    
    if (system(command) == 0) {
        printf("Backup created successfully: %s\n", backup_name);
    } else {
        printf("Backup creation failed.\n");
    }
}

/* -------------------- Security Functions -------------------- */
void simple_hash(const char *input, char *output) {
    const unsigned char *str = (const unsigned char*)input;
    unsigned long hash = 5381;
    int c;
    
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    
    snprintf(output, 65, "%016lx", hash);
}

int verify_password(const char *input_password, const char *stored_hash) {
    char computed_hash[65];
    simple_hash(input_password, computed_hash);
    return strcmp(computed_hash, stored_hash) == 0;
}

/* -------------------- User Management -------------------- */
int next_id_from_file(const char *file) {
    if (!file_exists(file)) return 1;
    
    FILE *f = fopen(file, "r");
    if (!f) return 1;
    
    char line[MAX_LINE];
    int maxid = 0;
    
    while (fgets(line, sizeof(line), f)) {
        char *id_str = parse_csv_field(line, 0);
        if (id_str) {
            int id = atoi(id_str);
            if (id > maxid) maxid = id;
        }
    }
    fclose(f);
    return maxid + 1;
}

void add_user(User *current_user) {
    if (!current_user->can_manage_users) {
        printf("Permission denied: You don't have permission to manage users.\n");
        return;
    }
    
    User new_user;
    new_user.id = next_id_from_file(USERS_FILE);
    
    printf("\n=== Add New User (ID: %d) ===\n", new_user.id);
    
    get_validated_string("Username: ", new_user.username, sizeof(new_user.username));
    
    // Check if username already exists
    FILE *check = fopen(USERS_FILE, "r");
    if (check) {
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), check)) {
            char *existing_username = parse_csv_field(line, 1);
            if (strcmp(existing_username, new_user.username) == 0) {
                printf("Error: Username already exists.\n");
                fclose(check);
                return;
            }
        }
        fclose(check);
    }
    
    char password[MAX_PASSWORD_LEN];
    printf("Password: ");
    if (fgets(password, sizeof(password), stdin) == NULL) return;
    trim_newline(password);
    
    if (strlen(password) < 4) {
        printf("Error: Password must be at least 4 characters long.\n");
        return;
    }
    
    simple_hash(password, new_user.password_hash);
    
    printf("\nSet Permissions (1 for Yes, 0 for No):\n");
    new_user.can_manage_products = get_validated_int("Can manage products? ", 0, 1);
    new_user.can_manage_customers = get_validated_int("Can manage customers? ", 0, 1);
    new_user.can_manage_sales = get_validated_int("Can manage sales? ", 0, 1);
    new_user.can_view_reports = get_validated_int("Can view reports? ", 0, 1);
    new_user.can_manage_users = get_validated_int("Can manage users? ", 0, 1);
    new_user.is_active = 1;
    
    FILE *f = fopen(USERS_FILE, "a");
    if (!f) {
        printf("Error: Unable to open users file.\n");
        return;
    }
    
    fprintf(f, "%d,%s,%s,%d,%d,%d,%d,%d,%d\n",
            new_user.id, new_user.username, new_user.password_hash,
            new_user.can_manage_products, new_user.can_manage_customers,
            new_user.can_manage_sales, new_user.can_view_reports,
            new_user.can_manage_users, new_user.is_active);
    fclose(f);
    
    printf("✓ User added successfully.\n");
}

void list_users(User *current_user) {
    if (!current_user->can_manage_users) {
        printf("Permission denied: You don't have permission to view users.\n");
        return;
    }
    
    if (!file_exists(USERS_FILE)) {
        printf("No users found.\n");
        return;
    }
    
    FILE *f = fopen(USERS_FILE, "r");
    if (!f) {
        printf("Error: Unable to read users file.\n");
        return;
    }
    
    char line[MAX_LINE];
    printf("\n%-4s %-15s %-8s %-8s %-8s %-8s %-8s %-8s\n",
           "ID", "Username", "Products", "Customers", "Sales", "Reports", "Users", "Active");
    printf("----------------------------------------------------------------\n");
    
    while (fgets(line, sizeof(line), f)) {
        User u;
        u.id = atoi(parse_csv_field(line, 0));
        strcpy(u.username, parse_csv_field(line, 1));
        strcpy(u.password_hash, parse_csv_field(line, 2));
        u.can_manage_products = atoi(parse_csv_field(line, 3));
        u.can_manage_customers = atoi(parse_csv_field(line, 4));
        u.can_manage_sales = atoi(parse_csv_field(line, 5));
        u.can_view_reports = atoi(parse_csv_field(line, 6));
        u.can_manage_users = atoi(parse_csv_field(line, 7));
        u.is_active = atoi(parse_csv_field(line, 8));
        
        printf("%-4d %-15s %-8s %-8s %-8s %-8s %-8s %-8s\n",
               u.id, u.username,
               u.can_manage_products ? "Yes" : "No",
               u.can_manage_customers ? "Yes" : "No",
               u.can_manage_sales ? "Yes" : "No",
               u.can_view_reports ? "Yes" : "No",
               u.can_manage_users ? "Yes" : "No",
               u.is_active ? "Yes" : "No");
    }
    fclose(f);
}

void delete_user(User *current_user) {
    if (!current_user->can_manage_users) {
        printf("Permission denied: You don't have permission to manage users.\n");
        return;
    }
    
    int user_id = get_validated_int("Enter user ID to delete: ", 1, 10000);
    
    // Prevent self-deletion
    if (user_id == current_user->id) {
        printf("Error: You cannot delete your own account.\n");
        return;
    }
    
    FILE *f = fopen(USERS_FILE, "r");
    FILE *tmp = fopen(".users_tmp", "w");
    if (!f || !tmp) {
        if (f) fclose(f);
        if (tmp) fclose(tmp);
        printf("Error: Unable to access users file.\n");
        return;
    }
    
    char line[MAX_LINE];
    int found = 0;
    
    while (fgets(line, sizeof(line), f)) {
        User u;
        u.id = atoi(parse_csv_field(line, 0));
        
        if (u.id == user_id) {
            found = 1;
            strcpy(u.username, parse_csv_field(line, 1));
            printf("Found user: %s (ID: %d)\n", u.username, u.id);
            
            char confirm[10];
            printf("Are you sure you want to delete this user? (yes/no): ");
            fgets(confirm, sizeof(confirm), stdin);
            trim_newline(confirm);
            
            if (strcasecmp(confirm, "yes") != 0 && strcasecmp(confirm, "y") != 0) {
                // Write the user back to file (keep them)
                fprintf(tmp, "%s", line);
                printf("Deletion cancelled.\n");
            } else {
                printf("User deleted successfully.\n");
            }
        } else {
            // Write other users to temp file
            fprintf(tmp, "%s", line);
        }
    }
    
    fclose(f);
    fclose(tmp);
    
    if (!found) {
        remove(".users_tmp");
        printf("Error: User ID %d not found.\n", user_id);
        return;
    }
    
    remove(USERS_FILE);
    rename(".users_tmp", USERS_FILE);
}

void edit_user_permissions(User *current_user) {
    if (!current_user->can_manage_users) {
        printf("Permission denied: You don't have permission to manage users.\n");
        return;
    }
    
    int user_id = get_validated_int("Enter user ID to edit: ", 1, 10000);
    
    FILE *f = fopen(USERS_FILE, "r");
    FILE *tmp = fopen(".users_tmp", "w");
    if (!f || !tmp) {
        if (f) fclose(f);
        if (tmp) fclose(tmp);
        printf("Error: Unable to access users file.\n");
        return;
    }
    
    char line[MAX_LINE];
    int found = 0;
    
    while (fgets(line, sizeof(line), f)) {
        User u;
        u.id = atoi(parse_csv_field(line, 0));
        strcpy(u.username, parse_csv_field(line, 1));
        strcpy(u.password_hash, parse_csv_field(line, 2));
        u.can_manage_products = atoi(parse_csv_field(line, 3));
        u.can_manage_customers = atoi(parse_csv_field(line, 4));
        u.can_manage_sales = atoi(parse_csv_field(line, 5));
        u.can_view_reports = atoi(parse_csv_field(line, 6));
        u.can_manage_users = atoi(parse_csv_field(line, 7));
        u.is_active = atoi(parse_csv_field(line, 8));
        
        if (u.id == user_id) {
            found = 1;
            printf("\nEditing user: %s (ID: %d)\n", u.username, u.id);
            printf("Current permissions:\n");
            printf("  Manage Products: %s\n", u.can_manage_products ? "Yes" : "No");
            printf("  Manage Customers: %s\n", u.can_manage_customers ? "Yes" : "No");
            printf("  Manage Sales: %s\n", u.can_manage_sales ? "Yes" : "No");
            printf("  View Reports: %s\n", u.can_view_reports ? "Yes" : "No");
            printf("  Manage Users: %s\n", u.can_manage_users ? "Yes" : "No");
            printf("  Active: %s\n", u.is_active ? "Yes" : "No");
            
            printf("\nSet new permissions (1 for Yes, 0 for No):\n");
            u.can_manage_products = get_validated_int("Can manage products? ", 0, 1);
            u.can_manage_customers = get_validated_int("Can manage customers? ", 0, 1);
            u.can_manage_sales = get_validated_int("Can manage sales? ", 0, 1);
            u.can_view_reports = get_validated_int("Can view reports? ", 0, 1);
            u.can_manage_users = get_validated_int("Can manage users? ", 0, 1);
            u.is_active = get_validated_int("Is active? ", 0, 1);
        }
        
        fprintf(tmp, "%d,%s,%s,%d,%d,%d,%d,%d,%d\n",
                u.id, u.username, u.password_hash,
                u.can_manage_products, u.can_manage_customers,
                u.can_manage_sales, u.can_view_reports,
                u.can_manage_users, u.is_active);
    }
    
    fclose(f);
    fclose(tmp);
    
    if (!found) {
        remove(".users_tmp");
        printf("Error: User ID %d not found.\n", user_id);
        return;
    }
    
    remove(USERS_FILE);
    rename(".users_tmp", USERS_FILE);
    printf("✓ User permissions updated successfully.\n");
}

void user_management_menu(User *current_user) {
    if (!current_user->can_manage_users) {
        printf("Permission denied: You don't have permission to manage users.\n");
        return;
    }
    
    int running = 1;
    while (running) {
        printf("\n=== User Management ===\n");
        printf("1. Add New User\n");
        printf("2. List All Users\n");
        printf("3. Edit User Permissions\n");
        printf("4. Delete User\n");
        printf("5. Return to Main Menu\n");
        
        int choice = get_validated_int("Select option: ", 1, 5);
        
        switch (choice) {
            case 1: add_user(current_user); break;
            case 2: list_users(current_user); break;
            case 3: edit_user_permissions(current_user); break;
            case 4: delete_user(current_user); break;
            case 5: running = 0; break;
        }
        
        if (running) pause_and_wait();
    }
}

/* -------------------- Product Functions -------------------- */
void add_product(User *current_user) {
    if (!current_user->can_manage_products) {
        printf("Permission denied: You don't have permission to manage products.\n");
        return;
    }
    
    Product p;
    p.id = next_id_from_file(PRODUCTS_FILE);
    
    printf("\n=== Add New Product (ID: %d) ===\n", p.id);
    
    get_validated_string("Product Name: ", p.name, sizeof(p.name));
    get_validated_string("Category: ", p.category, sizeof(p.category));
    get_validated_string("Brand: ", p.brand, sizeof(p.brand));
    p.cost_price = get_validated_float("Cost Price: ", 0);
    p.sell_price = get_validated_float("Sell Price: ", p.cost_price);
    p.stock = get_validated_int("Stock Quantity: ", 0, 10000);
    p.min_stock_level = get_validated_int("Minimum Stock Level: ", 0, 10000);

    FILE *f = fopen(PRODUCTS_FILE, "a");
    if (!f) { 
        printf("Error: Unable to open products file.\n"); 
        return; 
    }
    
    fprintf(f, "%d,\"%s\",\"%s\",\"%s\",%.2f,%.2f,%d,%d\n", 
            p.id, p.name, p.category, p.brand, p.cost_price, p.sell_price, p.stock, p.min_stock_level);
    fclose(f);
    
    printf("✓ Product added successfully.\n");
}

void list_products(User *current_user) {
    if (!file_exists(PRODUCTS_FILE)) { 
        printf("No products found.\n"); 
        return; 
    }
    
    FILE *f = fopen(PRODUCTS_FILE, "r");
    if (!f) {
        printf("Error: Unable to read products file.\n");
        return;
    }
    
    char line[MAX_LINE];
    printf("\n%-4s %-20s %-15s %-15s %-8s %-8s %-6s %-6s\n", 
           "ID", "Name", "Category", "Brand", "Cost", "Price", "Stock", "Min");
    printf("-------------------------------------------------------------------------------\n");
    
    while (fgets(line, sizeof(line), f)) {
        Product p;
        p.id = atoi(parse_csv_field(line, 0));
        strcpy(p.name, parse_csv_field(line, 1));
        strcpy(p.category, parse_csv_field(line, 2));
        strcpy(p.brand, parse_csv_field(line, 3));
        p.cost_price = atof(parse_csv_field(line, 4));
        p.sell_price = atof(parse_csv_field(line, 5));
        p.stock = atoi(parse_csv_field(line, 6));
        p.min_stock_level = atoi(parse_csv_field(line, 7));
        
        printf("%-4d %-20s %-15s %-15s %-8.2f %-8.2f %-6d %-6d\n", 
               p.id, p.name, p.category, p.brand, p.cost_price, p.sell_price, p.stock, p.min_stock_level);
    }
    fclose(f);
}

void search_products(User *current_user) {
    if (!file_exists(PRODUCTS_FILE)) { 
        printf("No products found.\n"); 
        return; 
    }
    
    char search_term[100];
    get_validated_string("Enter search term (name, category, or brand): ", search_term, sizeof(search_term));
    
    FILE *f = fopen(PRODUCTS_FILE, "r");
    if (!f) {
        printf("Error: Unable to read products file.\n");
        return;
    }
    
    char line[MAX_LINE];
    int found = 0;
    
    printf("\nSearch Results:\n");
    printf("%-4s %-20s %-15s %-15s %-8s %-8s %-6s\n", 
           "ID", "Name", "Category", "Brand", "Cost", "Price", "Stock");
    printf("----------------------------------------------------------------\n");
    
    while (fgets(line, sizeof(line), f)) {
        Product p;
        p.id = atoi(parse_csv_field(line, 0));
        strcpy(p.name, parse_csv_field(line, 1));
        strcpy(p.category, parse_csv_field(line, 2));
        strcpy(p.brand, parse_csv_field(line, 3));
        p.cost_price = atof(parse_csv_field(line, 4));
        p.sell_price = atof(parse_csv_field(line, 5));
        p.stock = atoi(parse_csv_field(line, 6));
        
        if (strstr(p.name, search_term) || strstr(p.category, search_term) || strstr(p.brand, search_term)) {
            printf("%-4d %-20s %-15s %-15s %-8.2f %-8.2f %-6d\n", 
                   p.id, p.name, p.category, p.brand, p.cost_price, p.sell_price, p.stock);
            found = 1;
        }
    }
    fclose(f);
    
    if (!found) {
        printf("No products found matching '%s'\n", search_term);
    }
}

int find_product_by_id(Product *out, int id) {
    if (!file_exists(PRODUCTS_FILE)) return 0;
    
    FILE *f = fopen(PRODUCTS_FILE, "r");
    if (!f) return 0;
    
    char line[MAX_LINE];
    int found = 0;
    
    while (fgets(line, sizeof(line), f)) {
        Product p;
        p.id = atoi(parse_csv_field(line, 0));
        
        if (p.id == id) {
            strcpy(p.name, parse_csv_field(line, 1));
            strcpy(p.category, parse_csv_field(line, 2));
            strcpy(p.brand, parse_csv_field(line, 3));
            p.cost_price = atof(parse_csv_field(line, 4));
            p.sell_price = atof(parse_csv_field(line, 5));
            p.stock = atoi(parse_csv_field(line, 6));
            p.min_stock_level = atoi(parse_csv_field(line, 7));
            
            if (out) *out = p;
            found = 1;
            break;
        }
    }
    fclose(f);
    return found;
}

void update_product_stock(int product_id, int delta) {
    if (!file_exists(PRODUCTS_FILE)) return;
    
    FILE *f = fopen(PRODUCTS_FILE, "r");
    FILE *tmp = fopen(".products_tmp", "w");
    if (!f || !tmp) {
        if (f) fclose(f);
        if (tmp) fclose(tmp);
        return;
    }
    
    char line[MAX_LINE];
    int updated = 0;
    
    while (fgets(line, sizeof(line), f)) {
        Product p;
        p.id = atoi(parse_csv_field(line, 0));
        
        if (p.id == product_id) {
            strcpy(p.name, parse_csv_field(line, 1));
            strcpy(p.category, parse_csv_field(line, 2));
            strcpy(p.brand, parse_csv_field(line, 3));
            p.cost_price = atof(parse_csv_field(line, 4));
            p.sell_price = atof(parse_csv_field(line, 5));
            p.stock = atoi(parse_csv_field(line, 6));
            p.min_stock_level = atoi(parse_csv_field(line, 7));
            
            p.stock += delta;
            if (p.stock < 0) p.stock = 0;
            updated = 1;
        } else {
            strcpy(p.name, parse_csv_field(line, 1));
            strcpy(p.category, parse_csv_field(line, 2));
            strcpy(p.brand, parse_csv_field(line, 3));
            p.cost_price = atof(parse_csv_field(line, 4));
            p.sell_price = atof(parse_csv_field(line, 5));
            p.stock = atoi(parse_csv_field(line, 6));
            p.min_stock_level = atoi(parse_csv_field(line, 7));
        }
        
        fprintf(tmp, "%d,\"%s\",\"%s\",\"%s\",%.2f,%.2f,%d,%d\n", 
                p.id, p.name, p.category, p.brand, p.cost_price, p.sell_price, p.stock, p.min_stock_level);
    }
    
    fclose(f);
    fclose(tmp);
    
    if (updated) {
        remove(PRODUCTS_FILE);
        rename(".products_tmp", PRODUCTS_FILE);
    } else {
        remove(".products_tmp");
    }
}

/* -------------------- Customer Functions -------------------- */
void add_customer(User *current_user) {
    if (!current_user->can_manage_customers) {
        printf("Permission denied: You don't have permission to manage customers.\n");
        return;
    }
    
    Customer c;
    c.id = next_id_from_file(CUSTOMERS_FILE);
    
    printf("\n=== Add New Customer (ID: %d) ===\n", c.id);
    
    get_validated_string("Full Name: ", c.name, sizeof(c.name));
    get_validated_string("Phone: ", c.phone, sizeof(c.phone));
    get_validated_string("Email: ", c.email, sizeof(c.email));
    get_validated_string("Address: ", c.address, sizeof(c.address));

    FILE *f = fopen(CUSTOMERS_FILE, "a");
    if (!f) { 
        printf("Error: Unable to open customers file.\n"); 
        return; 
    }
    
    fprintf(f, "%d,\"%s\",\"%s\",\"%s\",\"%s\"\n", 
            c.id, c.name, c.phone, c.email, c.address);
    fclose(f);
    
    printf("✓ Customer added successfully.\n");
}

void list_customers(User *current_user) {
    if (!file_exists(CUSTOMERS_FILE)) { 
        printf("No customers found.\n"); 
        return; 
    }
    
    FILE *f = fopen(CUSTOMERS_FILE, "r");
    if (!f) {
        printf("Error: Unable to read customers file.\n");
        return;
    }
    
    char line[MAX_LINE];
    printf("\n%-4s %-20s %-15s %-25s %-30s\n", 
           "ID", "Name", "Phone", "Email", "Address");
    printf("----------------------------------------------------------------------------------------\n");
    
    while (fgets(line, sizeof(line), f)) {
        Customer c;
        c.id = atoi(parse_csv_field(line, 0));
        strcpy(c.name, parse_csv_field(line, 1));
        strcpy(c.phone, parse_csv_field(line, 2));
        strcpy(c.email, parse_csv_field(line, 3));
        strcpy(c.address, parse_csv_field(line, 4));
        
        printf("%-4d %-20s %-15s %-25s %-30s\n", 
               c.id, c.name, c.phone, c.email, c.address);
    }
    fclose(f);
}

void search_customers(User *current_user) {
    if (!file_exists(CUSTOMERS_FILE)) { 
        printf("No customers found.\n"); 
        return; 
    }
    
    char search_term[100];
    get_validated_string("Enter search term (name, phone, or email): ", search_term, sizeof(search_term));
    
    FILE *f = fopen(CUSTOMERS_FILE, "r");
    if (!f) {
        printf("Error: Unable to read customers file.\n");
        return;
    }
    
    char line[MAX_LINE];
    int found = 0;
    
    printf("\nSearch Results:\n");
    printf("%-4s %-20s %-15s %-25s\n", "ID", "Name", "Phone", "Email");
    printf("----------------------------------------------------\n");
    
    while (fgets(line, sizeof(line), f)) {
        Customer c;
        c.id = atoi(parse_csv_field(line, 0));
        strcpy(c.name, parse_csv_field(line, 1));
        strcpy(c.phone, parse_csv_field(line, 2));
        strcpy(c.email, parse_csv_field(line, 3));
        
        if (strstr(c.name, search_term) || strstr(c.phone, search_term) || strstr(c.email, search_term)) {
            printf("%-4d %-20s %-15s %-25s\n", c.id, c.name, c.phone, c.email);
            found = 1;
        }
    }
    fclose(f);
    
    if (!found) {
        printf("No customers found matching '%s'\n", search_term);
    }
}

int find_customer_by_id(Customer *out, int id) {
    if (!file_exists(CUSTOMERS_FILE)) return 0;
    
    FILE *f = fopen(CUSTOMERS_FILE, "r");
    if (!f) return 0;
    
    char line[MAX_LINE];
    int found = 0;
    
    while (fgets(line, sizeof(line), f)) {
        Customer c;
        c.id = atoi(parse_csv_field(line, 0));
        
        if (c.id == id) {
            strcpy(c.name, parse_csv_field(line, 1));
            strcpy(c.phone, parse_csv_field(line, 2));
            strcpy(c.email, parse_csv_field(line, 3));
            strcpy(c.address, parse_csv_field(line, 4));
            
            if (out) *out = c;
            found = 1;
            break;
        }
    }
    fclose(f);
    return found;
}

/* -------------------- Sales Functions -------------------- */
void make_sale(User *current_user) {
    if (!current_user->can_manage_sales) {
        printf("Permission denied: You don't have permission to manage sales.\n");
        return;
    }
    
    if (!file_exists(PRODUCTS_FILE)) { 
        printf("No products available to sell.\n"); 
        return; 
    }
    
    Sale s;
    s.id = next_id_from_file(SALES_FILE);
    
    printf("\n=== Create New Sale (ID: %d) ===\n", s.id);
    
    list_products(current_user);
    
    int pid = get_validated_int("Enter product ID: ", 1, 10000);
    Product p;
    if (!find_product_by_id(&p, pid)) { 
        printf("Error: Product not found.\n"); 
        return; 
    }
    
    printf("Selected: %s (Stock: %d, Price: %.2f)\n", p.name, p.stock, p.sell_price);
    
    int cid = get_validated_int("Enter customer ID (0 to add new): ", 0, 10000);
    if (cid == 0) { 
        add_customer(current_user); 
        cid = next_id_from_file(CUSTOMERS_FILE) - 1; 
    }
    
    Customer cust;
    if (!find_customer_by_id(&cust, cid)) { 
        printf("Error: Customer not found.\n"); 
        return; 
    }
    
    int qty = get_validated_int("Quantity: ", 1, p.stock);
    
    s.product_id = pid;
    s.customer_id = cid;
    s.quantity = qty;
    s.total_price = p.sell_price * qty;
    now_str(s.date, sizeof(s.date));
    
    get_validated_string("Cashier name: ", s.cashier, sizeof(s.cashier));

    FILE *f = fopen(SALES_FILE, "a");
    if (!f) { 
        printf("Error: Unable to write sales file.\n"); 
        return; 
    }
    
    fprintf(f, "%d,%d,%d,%d,%.2f,\"%s\",\"%s\"\n", 
            s.id, s.product_id, s.customer_id, s.quantity, s.total_price, s.date, s.cashier);
    fclose(f);

    update_product_stock(pid, -qty);

    printf("\n✓ Sale recorded successfully!\n");
    printf("Product: %s\n", p.name);
    printf("Customer: %s\n", cust.name);
    printf("Quantity: %d\n", qty);
    printf("Total Amount: %.2f\n", s.total_price);
}

void list_sales(User *current_user) {
    if (!current_user->can_manage_sales) {
        printf("Permission denied: You don't have permission to view sales.\n");
        return;
    }
    
    if (!file_exists(SALES_FILE)) { 
        printf("No sales recorded.\n"); 
        return; 
    }
    
    FILE *f = fopen(SALES_FILE, "r");
    if (!f) {
        printf("Error: Unable to read sales file.\n");
        return;
    }
    
    char line[MAX_LINE];
    float total_revenue = 0;
    int total_sales = 0;
    
    printf("\n%-4s %-8s %-8s %-4s %-10s %-20s %-15s\n", 
           "ID", "ProdID", "CustID", "Qty", "Total", "Date", "Cashier");
    printf("----------------------------------------------------------------\n");
    
    while (fgets(line, sizeof(line), f)) {
        Sale s;
        s.id = atoi(parse_csv_field(line, 0));
        s.product_id = atoi(parse_csv_field(line, 1));
        s.customer_id = atoi(parse_csv_field(line, 2));
        s.quantity = atoi(parse_csv_field(line, 3));
        s.total_price = atof(parse_csv_field(line, 4));
        strcpy(s.date, parse_csv_field(line, 5));
        strcpy(s.cashier, parse_csv_field(line, 6));
        
        printf("%-4d %-8d %-8d %-4d %-10.2f %-20s %-15s\n", 
               s.id, s.product_id, s.customer_id, s.quantity, s.total_price, s.date, s.cashier);
        
        total_revenue += s.total_price;
        total_sales++;
    }
    fclose(f);
    
    printf("\nSummary: %d sales, Total Revenue: %.2f\n", total_sales, total_revenue);
}

/* -------------------- Reports -------------------- */
void report_low_stock(User *current_user) {
    if (!current_user->can_view_reports) {
        printf("Permission denied: You don't have permission to view reports.\n");
        return;
    }
    
    if (!file_exists(PRODUCTS_FILE)) { 
        printf("No products found.\n"); 
        return; 
    }
    
    int threshold = get_validated_int("Low stock threshold: ", 0, 10000);
    
    FILE *f = fopen(PRODUCTS_FILE, "r");
    if (!f) {
        printf("Error: Unable to read products file.\n");
        return;
    }
    
    char line[MAX_LINE];
    int low_stock_count = 0;
    
    printf("\nProducts with stock <= %d:\n", threshold);
    printf("%-4s %-20s %-15s %-6s %-6s\n", "ID", "Name", "Category", "Stock", "Min");
    printf("-------------------------------------------------\n");
    
    while (fgets(line, sizeof(line), f)) {
        Product p;
        p.id = atoi(parse_csv_field(line, 0));
        strcpy(p.name, parse_csv_field(line, 1));
        strcpy(p.category, parse_csv_field(line, 2));
        p.stock = atoi(parse_csv_field(line, 6));
        p.min_stock_level = atoi(parse_csv_field(line, 7));
        
        if (p.stock <= threshold) {
            printf("%-4d %-20s %-15s %-6d %-6d\n", 
                   p.id, p.name, p.category, p.stock, p.min_stock_level);
            low_stock_count++;
        }
    }
    fclose(f);
    
    printf("\nTotal low stock items: %d\n", low_stock_count);
}

void report_sales_summary(User *current_user) {
    if (!current_user->can_view_reports) {
        printf("Permission denied: You don't have permission to view reports.\n");
        return;
    }
    
    if (!file_exists(SALES_FILE)) { 
        printf("No sales recorded.\n"); 
        return; 
    }
    
    FILE *f = fopen(SALES_FILE, "r");
    if (!f) {
        printf("Error: Unable to read sales file.\n");
        return;
    }
    
    char line[MAX_LINE];
    float total_revenue = 0;
    int total_transactions = 0;
    int total_units = 0;
    
    while (fgets(line, sizeof(line), f)) {
        total_transactions++;
        total_units += atoi(parse_csv_field(line, 3));
        total_revenue += atof(parse_csv_field(line, 4));
    }
    fclose(f);
    
    printf("\n=== Sales Summary Report ===\n");
    printf("Total Transactions: %d\n", total_transactions);
    printf("Total Units Sold: %d\n", total_units);
    printf("Total Revenue: %.2f\n", total_revenue);
    printf("Average Sale Value: %.2f\n", total_transactions > 0 ? total_revenue / total_transactions : 0);
}

void report_profit_analysis(User *current_user) {
    if (!current_user->can_view_reports) {
        printf("Permission denied: You don't have permission to view reports.\n");
        return;
    }
    
    if (!file_exists(SALES_FILE) || !file_exists(PRODUCTS_FILE)) { 
        printf("Insufficient data for profit analysis.\n"); 
        return; 
    }
    
    FILE *sales_file = fopen(SALES_FILE, "r");
    FILE *products_file = fopen(PRODUCTS_FILE, "r");
    
    if (!sales_file || !products_file) {
        if (sales_file) fclose(sales_file);
        if (products_file) fclose(products_file);
        printf("Error: Unable to access data files.\n");
        return;
    }
    
    float product_costs[10000] = {0};
    char product_line[MAX_LINE];
    
    while (fgets(product_line, sizeof(product_line), products_file)) {
        int id = atoi(parse_csv_field(product_line, 0));
        float cost = atof(parse_csv_field(product_line, 4));
        product_costs[id] = cost;
    }
    fclose(products_file);
    
    char sales_line[MAX_LINE];
    float total_revenue = 0;
    float total_cost = 0;
    int transactions = 0;
    
    while (fgets(sales_line, sizeof(sales_line), sales_file)) {
        int product_id = atoi(parse_csv_field(sales_line, 1));
        int quantity = atoi(parse_csv_field(sales_line, 3));
        float revenue = atof(parse_csv_field(sales_line, 4));
        
        total_revenue += revenue;
        total_cost += product_costs[product_id] * quantity;
        transactions++;
    }
    fclose(sales_file);
    
    float total_profit = total_revenue - total_cost;
    float profit_margin = total_revenue > 0 ? (total_profit / total_revenue) * 100 : 0;
    
    printf("\n=== Profit Analysis Report ===\n");
    printf("Total Transactions: %d\n", transactions);
    printf("Total Revenue: %.2f\n", total_revenue);
    printf("Total Cost: %.2f\n", total_cost);
    printf("Total Profit: %.2f\n", total_profit);
    printf("Profit Margin: %.2f%%\n", profit_margin);
}

/* -------------------- Authentication -------------------- */
int ensure_default_user() {
    if (!file_exists(USERS_FILE)) {
        FILE *f = fopen(USERS_FILE, "w");
        if (!f) return 0;
        
        User admin;
        admin.id = 1;
        strcpy(admin.username, "admin");
        simple_hash("admin", admin.password_hash);
        admin.can_manage_products = 1;
        admin.can_manage_customers = 1;
        admin.can_manage_sales = 1;
        admin.can_view_reports = 1;
        admin.can_manage_users = 1;
        admin.is_active = 1;
        
        fprintf(f, "%d,%s,%s,%d,%d,%d,%d,%d,%d\n",
                admin.id, admin.username, admin.password_hash,
                admin.can_manage_products, admin.can_manage_customers,
                admin.can_manage_sales, admin.can_view_reports,
                admin.can_manage_users, admin.is_active);
        fclose(f);
    }
    return 1;
}

int login(User *current_user) {
    if (!ensure_default_user()) {
        printf("Error: Cannot initialize user system.\n");
        return 0;
    }
    
    char username[50];
    char password[MAX_PASSWORD_LEN];
    
    printf("\n=== Shop Manager Login ===\n");
    
    get_validated_string("Username: ", username, sizeof(username));
    
    printf("Password: ");
    if (fgets(password, sizeof(password), stdin) == NULL) {
        return 0;
    }
    trim_newline(password);
    
    FILE *f = fopen(USERS_FILE, "r");
    if (!f) return 0;
    
    char line[MAX_LINE];
    int authenticated = 0;
    
    while (fgets(line, sizeof(line), f)) {
        User u;
        u.id = atoi(parse_csv_field(line, 0));
        strcpy(u.username, parse_csv_field(line, 1));
        strcpy(u.password_hash, parse_csv_field(line, 2));
        u.can_manage_products = atoi(parse_csv_field(line, 3));
        u.can_manage_customers = atoi(parse_csv_field(line, 4));
        u.can_manage_sales = atoi(parse_csv_field(line, 5));
        u.can_view_reports = atoi(parse_csv_field(line, 6));
        u.can_manage_users = atoi(parse_csv_field(line, 7));
        u.is_active = atoi(parse_csv_field(line, 8));
        
        if (strcmp(u.username, username) == 0 && verify_password(password, u.password_hash) && u.is_active) {
            *current_user = u;
            authenticated = 1;
            printf("\nWelcome, %s!\n", u.username);
            printf("Permissions: %s%s%s%s%s\n",
                   u.can_manage_products ? "Products " : "",
                   u.can_manage_customers ? "Customers " : "",
                   u.can_manage_sales ? "Sales " : "",
                   u.can_view_reports ? "Reports " : "",
                   u.can_manage_users ? "Users" : "");
            break;
        }
    }
    fclose(f);
    
    if (!authenticated) {
        printf("Invalid username or password, or account is inactive.\n");
    }
    
    return authenticated;
}

void change_password(User *current_user) {
    char old_password[MAX_PASSWORD_LEN], new_password[MAX_PASSWORD_LEN];
    
    printf("\n=== Change Password ===\n");
    
    printf("Current Password: ");
    if (fgets(old_password, sizeof(old_password), stdin) == NULL) return;
    trim_newline(old_password);
    
    if (!verify_password(old_password, current_user->password_hash)) {
        printf("Error: Current password is incorrect.\n");
        return;
    }
    
    printf("New Password: ");
    if (fgets(new_password, sizeof(new_password), stdin) == NULL) return;
    trim_newline(new_password);
    
    if (strlen(new_password) < 4) {
        printf("Error: Password must be at least 4 characters long.\n");
        return;
    }
    
    FILE *f = fopen(USERS_FILE, "r");
    FILE *tmp = fopen(".users_tmp", "w");
    if (!f || !tmp) {
        if (f) fclose(f);
        if (tmp) fclose(tmp);
        printf("Error: Unable to access user database.\n");
        return;
    }
    
    char line[MAX_LINE];
    char new_hash[65];
    simple_hash(new_password, new_hash);
    
    while (fgets(line, sizeof(line), f)) {
        User u;
        u.id = atoi(parse_csv_field(line, 0));
        strcpy(u.username, parse_csv_field(line, 1));
        strcpy(u.password_hash, parse_csv_field(line, 2));
        u.can_manage_products = atoi(parse_csv_field(line, 3));
        u.can_manage_customers = atoi(parse_csv_field(line, 4));
        u.can_manage_sales = atoi(parse_csv_field(line, 5));
        u.can_view_reports = atoi(parse_csv_field(line, 6));
        u.can_manage_users = atoi(parse_csv_field(line, 7));
        u.is_active = atoi(parse_csv_field(line, 8));
        
        if (u.id == current_user->id) {
            strcpy(u.password_hash, new_hash);
            strcpy(current_user->password_hash, new_hash);
        }
        
        fprintf(tmp, "%d,%s,%s,%d,%d,%d,%d,%d,%d\n",
                u.id, u.username, u.password_hash,
                u.can_manage_products, u.can_manage_customers,
                u.can_manage_sales, u.can_view_reports,
                u.can_manage_users, u.is_active);
    }
    
    fclose(f);
    fclose(tmp);
    
    remove(USERS_FILE);
    rename(".users_tmp", USERS_FILE);
    printf("✓ Password changed successfully.\n");
}

/* -------------------- System Management -------------------- */
void system_maintenance(User *current_user) {
    printf("\n=== System Maintenance ===\n");
    printf("1. Create Backup\n");
    printf("2. Change Password\n");
    printf("3. Return to Main Menu\n");
    
    int choice = get_validated_int("Select option: ", 1, 3);
    
    switch (choice) {
        case 1:
            create_backup();
            break;
        case 2:
            change_password(current_user);
            break;
        case 3:
            return;
    }
    
    pause_and_wait();
}

/* -------------------- Main Menu -------------------- */
void show_main_menu(User *current_user) {
    printf("\n========= Shop Manager =========\n");
    printf("Logged in as: %s\n", current_user->username);
    printf("1. Products Management\n");
    printf("2. Customers Management\n");
    printf("3. Sales Management\n");
    printf("4. Reports & Analytics\n");
    if (current_user->can_manage_users) {
        printf("5. User Management\n");
    }
    printf("6. System Maintenance\n");
    printf("7. Exit\n");
    printf("Select option: ");
}

void products_menu(User *current_user) {
    if (!current_user->can_manage_products) {
        printf("Permission denied: You don't have permission to manage products.\n");
        return;
    }
    
    int running = 1;
    while (running) {
        printf("\n=== Products Management ===\n");
        printf("1. Add New Product\n");
        printf("2. List All Products\n");
        printf("3. Search Products\n");
        printf("4. Return to Main Menu\n");
        
        int choice = get_validated_int("Select option: ", 1, 4);
        
        switch (choice) {
            case 1: add_product(current_user); break;
            case 2: list_products(current_user); break;
            case 3: search_products(current_user); break;
            case 4: running = 0; break;
        }
        
        if (running) pause_and_wait();
    }
}

void customers_menu(User *current_user) {
    if (!current_user->can_manage_customers) {
        printf("Permission denied: You don't have permission to manage customers.\n");
        return;
    }
    
    int running = 1;
    while (running) {
        printf("\n=== Customers Management ===\n");
        printf("1. Add New Customer\n");
        printf("2. List All Customers\n");
        printf("3. Search Customers\n");
        printf("4. Return to Main Menu\n");
        
        int choice = get_validated_int("Select option: ", 1, 4);
        
        switch (choice) {
            case 1: add_customer(current_user); break;
            case 2: list_customers(current_user); break;
            case 3: search_customers(current_user); break;
            case 4: running = 0; break;
        }
        
        if (running) pause_and_wait();
    }
}

void sales_menu(User *current_user) {
    if (!current_user->can_manage_sales) {
        printf("Permission denied: You don't have permission to manage sales.\n");
        return;
    }
    
    int running = 1;
    while (running) {
        printf("\n=== Sales Management ===\n");
        printf("1. Make New Sale\n");
        printf("2. List All Sales\n");
        printf("3. Return to Main Menu\n");
        
        int choice = get_validated_int("Select option: ", 1, 3);
        
        switch (choice) {
            case 1: make_sale(current_user); break;
            case 2: list_sales(current_user); break;
            case 3: running = 0; break;
        }
        
        if (running) pause_and_wait();
    }
}

void reports_menu(User *current_user) {
    if (!current_user->can_view_reports) {
        printf("Permission denied: You don't have permission to view reports.\n");
        return;
    }
    
    int running = 1;
    while (running) {
        printf("\n=== Reports & Analytics ===\n");
        printf("1. Low Stock Report\n");
        printf("2. Sales Summary\n");
        printf("3. Profit Analysis\n");
        printf("4. Return to Main Menu\n");
        
        int choice = get_validated_int("Select option: ", 1, 4);
        
        switch (choice) {
            case 1: report_low_stock(current_user); break;
            case 2: report_sales_summary(current_user); break;
            case 3: report_profit_analysis(current_user); break;
            case 4: running = 0; break;
        }
        
        if (running) pause_and_wait();
    }
}

int main() {
    printf("Welcome to Enhanced Shop Manager\n");
    printf("================================\n");
    
    User current_user;
    if (!login(&current_user)) {
        printf("Login failed. Exiting.\n");
        return 1;
    }
    
    int running = 1;
    while (running) {
        show_main_menu(&current_user);
        int choice = get_validated_int("", 1, 7);
        
        switch (choice) {
            case 1: products_menu(&current_user); break;
            case 2: customers_menu(&current_user); break;
            case 3: sales_menu(&current_user); break;
            case 4: reports_menu(&current_user); break;
            case 5: 
                if (current_user.can_manage_users) {
                    user_management_menu(&current_user); 
                } else {
                    printf("Invalid choice.\n");
                }
                break;
            case 6: system_maintenance(&current_user); break;
            case 7: running = 0; break;
            default: printf("Invalid choice.\n"); break;
        }
    }
    
    printf("\nThank you for using Shop Manager. Goodbye!\n");
    return 0;
}