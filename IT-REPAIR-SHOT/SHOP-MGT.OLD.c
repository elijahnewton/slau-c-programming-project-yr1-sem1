/*
 * shop_manager.c
 * Command-line Management System for a Computer & Accessories Sales, Assembling and Repair Shop
 * Single-file C program using CSV files for persistence.
 * Features:
 *  - Product inventory (add, list, update stock, search)
 *  - Customers (add, list, search)
 *  - Sales (create sale, reduce stock, sales records)
 *  - Repairs (create repair job, update status, list)
 *  - Assemblies (create assembly order, bill, list)
 *  - Simple user authentication (admin default password: admin)
 *  - Basic reports: inventory low-stock, sales summary
 *
 * Compile: gcc -o shop_manager shop_manager.c
 * Run: ./shop_manager
 *
 * Data files (will be created in the program working directory):
 *  - products.csv
 *  - customers.csv
 *  - sales.csv
 *  - repairs.csv
 *  - assemblies.csv
 *  - users.csv
 *
 * Notes:
 *  - This is a starting point and is written to be readable and extendable.
 *  - Inputs are minimally validated for brevity; improve validation for production use.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_LINE 512
#define MAX_FIELD 128
#define PRODUCTS_FILE "products.csv"
#define CUSTOMERS_FILE "customers.csv"
#define SALES_FILE "sales.csv"
#define REPAIRS_FILE "repairs.csv"
#define ASSEMBLIES_FILE "assemblies.csv"
#define USERS_FILE "users.csv"

/* -------------------- Data Structures -------------------- */
typedef struct {
    int id;
    char name[100];
    char category[50];
    char brand[50];
    float cost_price;
    float sell_price;
    int stock;
} Product;

typedef struct {
    int id;
    char name[100];
    char phone[30];
    char email[100];
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
    int customer_id;
    char device[100];
    char problem[200];
    char status[50]; // e.g., Received, In Progress, Completed, Collected
    float cost_estimate;
    char date_received[64];
    char date_completed[64];
} Repair;

typedef struct {
    int id;
    int customer_id;
    char description[200];
    float price;
    char status[50]; // Pending, Assembled, Delivered
    char date[64];
} Assembly;

/* -------------------- Utility Helpers -------------------- */
void pause_and_wait() {
    printf("\nPress ENTER to continue...");
    while (getchar() != '\n');
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

int next_id_from_file(const char *file) {
    if (!file_exists(file)) return 1;
    FILE *f = fopen(file, "r");
    if (!f) return 1;
    char line[MAX_LINE];
    int maxid = 0;
    while (fgets(line, sizeof(line), f)) {
        char *p = strtok(line, ",");
        if (p) {
            int id = atoi(p);
            if (id > maxid) maxid = id;
        }
    }
    fclose(f);
    return maxid + 1;
}

/* -------------------- Product Functions -------------------- */
void add_product() {
    Product p;
    p.id = next_id_from_file(PRODUCTS_FILE);
    printf("Add new product (ID %d)\n", p.id);
    printf("Name: "); fgets(p.name, sizeof(p.name), stdin); p.name[strcspn(p.name, "\n")] = 0;
    printf("Category: "); fgets(p.category, sizeof(p.category), stdin); p.category[strcspn(p.category, "\n")] = 0;
    printf("Brand: "); fgets(p.brand, sizeof(p.brand), stdin); p.brand[strcspn(p.brand, "\n")] = 0;
    printf("Cost price: "); scanf("%f", &p.cost_price); getchar();
    printf("Sell price: "); scanf("%f", &p.sell_price); getchar();
    printf("Stock quantity: "); scanf("%d", &p.stock); getchar();

    FILE *f = fopen(PRODUCTS_FILE, "a");
    if (!f) { perror("Unable to open products file"); return; }
    fprintf(f, "%d,%s,%s,%s,%.2f,%.2f,%d\n", p.id, p.name, p.category, p.brand, p.cost_price, p.sell_price, p.stock);
    fclose(f);
    printf("Product added.\n");
}

void list_products() {
    if (!file_exists(PRODUCTS_FILE)) { printf("No products yet.\n"); return; }
    FILE *f = fopen(PRODUCTS_FILE, "r");
    char line[MAX_LINE];
    printf("\nID | Name | Category | Brand | Cost | Sell | Stock\n");
    printf("----------------------------------------------------------------\n");
    while (fgets(line, sizeof(line), f)) {
        Product p; char *tok;
        tok = strtok(line, ","); if (!tok) continue; p.id = atoi(tok);
        tok = strtok(NULL, ","); strcpy(p.name, tok?tok:"");
        tok = strtok(NULL, ","); strcpy(p.category, tok?tok:"");
        tok = strtok(NULL, ","); strcpy(p.brand, tok?tok:"");
        tok = strtok(NULL, ","); p.cost_price = tok?atof(tok):0;
        tok = strtok(NULL, ","); p.sell_price = tok?atof(tok):0;
        tok = strtok(NULL, ",\n"); p.stock = tok?atoi(tok):0;
        printf("%d | %s | %s | %s | %.2f | %.2f | %d\n", p.id, p.name, p.category, p.brand, p.cost_price, p.sell_price, p.stock);
    }
    fclose(f);
}

int find_product_by_id(Product *out, int id) {
    if (!file_exists(PRODUCTS_FILE)) return 0;
    FILE *f = fopen(PRODUCTS_FILE, "r"); char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        Product p; char *tok;
        tok = strtok(line, ","); if (!tok) continue; p.id = atoi(tok);
        if (p.id != id) continue;
        tok = strtok(NULL, ","); strcpy(p.name, tok?tok:"");
        tok = strtok(NULL, ","); strcpy(p.category, tok?tok:"");
        tok = strtok(NULL, ","); strcpy(p.brand, tok?tok:"");
        tok = strtok(NULL, ","); p.cost_price = tok?atof(tok):0;
        tok = strtok(NULL, ","); p.sell_price = tok?atof(tok):0;
        tok = strtok(NULL, ",\n"); p.stock = tok?atoi(tok):0;
        if (out) *out = p;
        fclose(f);
        return 1;
    }
    fclose(f);
    return 0;
}

void update_product_stock(int product_id, int delta) {
    if (!file_exists(PRODUCTS_FILE)) return;
    FILE *f = fopen(PRODUCTS_FILE, "r");
    FILE *tmp = fopen(".products_tmp","w");
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        Product p; char copy[MAX_LINE]; strcpy(copy, line);
        char *tok = strtok(copy, ","); if (!tok) continue; p.id = atoi(tok);
        tok = strtok(NULL, ","); strcpy(p.name, tok?tok:"");
        tok = strtok(NULL, ","); strcpy(p.category, tok?tok:"");
        tok = strtok(NULL, ","); strcpy(p.brand, tok?tok:"");
        tok = strtok(NULL, ","); p.cost_price = tok?atof(tok):0;
        tok = strtok(NULL, ","); p.sell_price = tok?atof(tok):0;
        tok = strtok(NULL, ",\n"); p.stock = tok?atoi(tok):0;
        if (p.id == product_id) {
            p.stock += delta;
            if (p.stock < 0) p.stock = 0;
        }
        fprintf(tmp, "%d,%s,%s,%s,%.2f,%.2f,%d\n", p.id, p.name, p.category, p.brand, p.cost_price, p.sell_price, p.stock);
    }
    fclose(f); fclose(tmp);
    remove(PRODUCTS_FILE);
    rename(".products_tmp", PRODUCTS_FILE);
}

/* -------------------- Customer Functions -------------------- */
void add_customer() {
    Customer c; c.id = next_id_from_file(CUSTOMERS_FILE);
    printf("Add customer (ID %d)\n", c.id);
    printf("Name: "); fgets(c.name, sizeof(c.name), stdin); c.name[strcspn(c.name, "\n")] = 0;
    printf("Phone: "); fgets(c.phone, sizeof(c.phone), stdin); c.phone[strcspn(c.phone, "\n")] = 0;
    printf("Email: "); fgets(c.email, sizeof(c.email), stdin); c.email[strcspn(c.email, "\n")] = 0;
    FILE *f = fopen(CUSTOMERS_FILE, "a");
    if (!f) { perror("Unable to open customers file"); return; }
    fprintf(f, "%d,%s,%s,%s\n", c.id, c.name, c.phone, c.email);
    fclose(f);
    printf("Customer added.\n");
}

void list_customers() {
    if (!file_exists(CUSTOMERS_FILE)) { printf("No customers yet.\n"); return; }
    FILE *f = fopen(CUSTOMERS_FILE, "r"); char line[MAX_LINE];
    printf("ID | Name | Phone | Email\n");
    printf("-----------------------------------------\n");
    while (fgets(line, sizeof(line), f)) {
        Customer c; char *tok;
        tok = strtok(line, ","); if (!tok) continue; c.id = atoi(tok);
        tok = strtok(NULL, ","); strcpy(c.name, tok?tok:"");
        tok = strtok(NULL, ","); strcpy(c.phone, tok?tok:"");
        tok = strtok(NULL, "\n"); strcpy(c.email, tok?tok:"");
        printf("%d | %s | %s | %s\n", c.id, c.name, c.phone, c.email);
    }
    fclose(f);
}

int find_customer_by_id(Customer *out, int id) {
    if (!file_exists(CUSTOMERS_FILE)) return 0;
    FILE *f = fopen(CUSTOMERS_FILE, "r"); char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        Customer c; char *tok;
        tok = strtok(line, ","); if (!tok) continue; c.id = atoi(tok);
        if (c.id != id) continue;
        tok = strtok(NULL, ","); strcpy(c.name, tok?tok:"");
        tok = strtok(NULL, ","); strcpy(c.phone, tok?tok:"");
        tok = strtok(NULL, "\n"); strcpy(c.email, tok?tok:"");
        if (out) *out = c;
        fclose(f);
        return 1;
    }
    fclose(f);
    return 0;
}

/* -------------------- Sales Functions -------------------- */
void make_sale() {
    if (!file_exists(PRODUCTS_FILE)) { printf("No products available to sell.\n"); return; }
    if (!file_exists(CUSTOMERS_FILE)) { printf("No customers in system. Add a customer first.\n"); return; }
    Sale s; s.id = next_id_from_file(SALES_FILE);
    int pid, cid, qty;
    printf("Create Sale (ID %d)\n", s.id);
    list_products();
    printf("Enter product ID: "); scanf("%d", &pid); getchar();
    Product p;
    if (!find_product_by_id(&p, pid)) { printf("Product not found.\n"); return; }
    printf("Enter customer ID (or 0 to add new): "); scanf("%d", &cid); getchar();
    if (cid == 0) { add_customer(); cid = next_id_from_file(CUSTOMERS_FILE) - 1; }
    if (!find_customer_by_id(NULL, cid)) { printf("Customer not found.\n"); return; }
    printf("Quantity: "); scanf("%d", &qty); getchar();
    if (qty > p.stock) { printf("Not enough stock. Available: %d\n", p.stock); return; }
    s.product_id = pid; s.customer_id = cid; s.quantity = qty;
    s.total_price = p.sell_price * qty;
    now_str(s.date, sizeof(s.date));
    printf("Cashier name: "); fgets(s.cashier, sizeof(s.cashier), stdin); s.cashier[strcspn(s.cashier, "\n")] = 0;

    FILE *f = fopen(SALES_FILE, "a");
    if (!f) { perror("Unable to write sales file"); return; }
    fprintf(f, "%d,%d,%d,%d,%.2f,%s,%s\n", s.id, s.product_id, s.customer_id, s.quantity, s.total_price, s.date, s.cashier);
    fclose(f);

    update_product_stock(pid, -qty);

    printf("Sale recorded. Total: %.2f\n", s.total_price);
}

void list_sales() {
    if (!file_exists(SALES_FILE)) { printf("No sales recorded.\n"); return; }
    FILE *f = fopen(SALES_FILE, "r"); char line[MAX_LINE];
    printf("ID | ProdID | CustID | Qty | Total | Date | Cashier\n");
    printf("---------------------------------------------------------\n");
    while (fgets(line, sizeof(line), f)) {
        Sale s; char *tok;
        tok = strtok(line, ","); if (!tok) continue; s.id = atoi(tok);
        tok = strtok(NULL, ","); s.product_id = tok?atoi(tok):0;
        tok = strtok(NULL, ","); s.customer_id = tok?atoi(tok):0;
        tok = strtok(NULL, ","); s.quantity = tok?atoi(tok):0;
        tok = strtok(NULL, ","); s.total_price = tok?atof(tok):0;
        tok = strtok(NULL, ","); strcpy(s.date, tok?tok:"");
        tok = strtok(NULL, "\n"); strcpy(s.cashier, tok?tok:"");
        printf("%d | %d | %d | %d | %.2f | %s | %s\n", s.id, s.product_id, s.customer_id, s.quantity, s.total_price, s.date, s.cashier);
    }
    fclose(f);
}

/* -------------------- Repair Functions -------------------- */
void create_repair_job() {
    Repair r; r.id = next_id_from_file(REPAIRS_FILE);
    printf("Create repair job (ID %d)\n", r.id);
    printf("Customer ID (or 0 to add new): "); scanf("%d", &r.customer_id); getchar();
    if (r.customer_id == 0) { add_customer(); r.customer_id = next_id_from_file(CUSTOMERS_FILE) - 1; }
    if (!find_customer_by_id(NULL, r.customer_id)) { printf("Customer not found.\n"); return; }
    printf("Device (e.g., Laptop Model X): "); fgets(r.device, sizeof(r.device), stdin); r.device[strcspn(r.device, "\n")] = 0;
    printf("Problem description: "); fgets(r.problem, sizeof(r.problem), stdin); r.problem[strcspn(r.problem, "\n")] = 0;
    r.cost_estimate = 0; r.date_completed[0]=0;
    printf("Cost estimate: "); scanf("%f", &r.cost_estimate); getchar();
    strcpy(r.status, "Received");
    now_str(r.date_received, sizeof(r.date_received));
    FILE *f = fopen(REPAIRS_FILE, "a");
    if (!f) { perror("Unable to write repairs file"); return; }
    fprintf(f, "%d,%d,%s,%s,%s,%.2f,%s,%s\n", r.id, r.customer_id, r.device, r.problem, r.status, r.cost_estimate, r.date_received, r.date_completed);
    fclose(f);
    printf("Repair job created.\n");
}

void list_repairs() {
    if (!file_exists(REPAIRS_FILE)) { printf("No repairs recorded.\n"); return; }
    FILE *f = fopen(REPAIRS_FILE, "r"); char line[MAX_LINE];
    printf("ID | CustID | Device | Problem | Status | EstCost | Received | Completed\n");
    printf("--------------------------------------------------------------------------------\n");
    while (fgets(line, sizeof(line), f)) {
        Repair r; char *tok;
        tok = strtok(line, ","); if (!tok) continue; r.id = atoi(tok);
        tok = strtok(NULL, ","); r.customer_id = tok?atoi(tok):0;
        tok = strtok(NULL, ","); strcpy(r.device, tok?tok:"");
        tok = strtok(NULL, ","); strcpy(r.problem, tok?tok:"");
        tok = strtok(NULL, ","); strcpy(r.status, tok?tok:"");
        tok = strtok(NULL, ","); r.cost_estimate = tok?atof(tok):0;
        tok = strtok(NULL, ","); strcpy(r.date_received, tok?tok:"");
        tok = strtok(NULL, "\n"); strcpy(r.date_completed, tok?tok:"");
        printf("%d | %d | %s | %s | %s | %.2f | %s | %s\n", r.id, r.customer_id, r.device, r.problem, r.status, r.cost_estimate, r.date_received, r.date_completed);
    }
    fclose(f);
}

void update_repair_status() {
    if (!file_exists(REPAIRS_FILE)) { printf("No repairs recorded.\n"); return; }
    int id; printf("Enter repair ID: "); scanf("%d", &id); getchar();
    FILE *f = fopen(REPAIRS_FILE, "r"); FILE *tmp = fopen(".repairs_tmp","w"); char line[MAX_LINE];
    int found = 0;
    while (fgets(line, sizeof(line), f)) {
        Repair r; char copy[MAX_LINE]; strcpy(copy, line);
        char *tok = strtok(copy, ","); if (!tok) continue; r.id = atoi(tok);
        tok = strtok(NULL, ","); r.customer_id = tok?atoi(tok):0;
        tok = strtok(NULL, ","); strcpy(r.device, tok?tok:"");
        tok = strtok(NULL, ","); strcpy(r.problem, tok?tok:"");
        tok = strtok(NULL, ","); strcpy(r.status, tok?tok:"");
        tok = strtok(NULL, ","); r.cost_estimate = tok?atof(tok):0;
        tok = strtok(NULL, ","); strcpy(r.date_received, tok?tok:"");
        tok = strtok(NULL, "\n"); strcpy(r.date_completed, tok?tok:"");
        if (r.id == id) {
            found = 1;
            printf("Current status: %s\n", r.status);
            printf("New status (Received, In Progress, Completed, Collected): "); fgets(r.status, sizeof(r.status), stdin); r.status[strcspn(r.status, "\n")] = 0;
            if (strcmp(r.status, "Completed") == 0) {
                now_str(r.date_completed, sizeof(r.date_completed));
            }
        }
        fprintf(tmp, "%d,%d,%s,%s,%s,%.2f,%s,%s\n", r.id, r.customer_id, r.device, r.problem, r.status, r.cost_estimate, r.date_received, r.date_completed);
    }
    fclose(f); fclose(tmp);
    if (!found) { remove(".repairs_tmp"); printf("Repair ID not found.\n"); return; }
    remove(REPAIRS_FILE); rename(".repairs_tmp", REPAIRS_FILE);
    printf("Repair updated.\n");
}

/* -------------------- Assembly Functions -------------------- */
void create_assembly_order() {
    Assembly a; a.id = next_id_from_file(ASSEMBLIES_FILE);
    printf("Create assembly order (ID %d)\n", a.id);
    printf("Customer ID (or 0 to add new): "); scanf("%d", &a.customer_id); getchar();
    if (a.customer_id == 0) { add_customer(); a.customer_id = next_id_from_file(CUSTOMERS_FILE) - 1; }
    if (!find_customer_by_id(NULL, a.customer_id)) { printf("Customer not found.\n"); return; }
    printf("Description (components / notes): "); fgets(a.description, sizeof(a.description), stdin); a.description[strcspn(a.description, "\n")] = 0;
    printf("Price (labor + parts): "); scanf("%f", &a.price); getchar();
    strcpy(a.status, "Pending"); now_str(a.date, sizeof(a.date));
    FILE *f = fopen(ASSEMBLIES_FILE, "a");
    if (!f) { perror("Unable to write assemblies file"); return; }
    fprintf(f, "%d,%d,%s,%.2f,%s,%s\n", a.id, a.customer_id, a.description, a.price, a.status, a.date);
    fclose(f);
    printf("Assembly order created.\n");
}

void list_assemblies() {
    if (!file_exists(ASSEMBLIES_FILE)) { printf("No assembly orders yet.\n"); return; }
    FILE *f = fopen(ASSEMBLIES_FILE, "r"); char line[MAX_LINE];
    printf("ID | CustID | Desc | Price | Status | Date\n");
    printf("----------------------------------------------------------------\n");
    while (fgets(line, sizeof(line), f)) {
        Assembly a; char *tok;
        tok = strtok(line, ","); if (!tok) continue; a.id = atoi(tok);
        tok = strtok(NULL, ","); a.customer_id = tok?atoi(tok):0;
        tok = strtok(NULL, ","); strcpy(a.description, tok?tok:"");
        tok = strtok(NULL, ","); a.price = tok?atof(tok):0;
        tok = strtok(NULL, ","); strcpy(a.status, tok?tok:"");
        tok = strtok(NULL, "\n"); strcpy(a.date, tok?tok:"");
        printf("%d | %d | %s | %.2f | %s | %s\n", a.id, a.customer_id, a.description, a.price, a.status, a.date);
    }
    fclose(f);
}

void update_assembly_status() {
    if (!file_exists(ASSEMBLIES_FILE)) { printf("No assembly orders yet.\n"); return; }
    int id; printf("Enter assembly ID: "); scanf("%d", &id); getchar();
    FILE *f = fopen(ASSEMBLIES_FILE, "r"); FILE *tmp = fopen(".assemblies_tmp","w"); char line[MAX_LINE];
    int found = 0;
    while (fgets(line, sizeof(line), f)) {
        Assembly a; char copy[MAX_LINE]; strcpy(copy, line);
        char *tok = strtok(copy, ","); if (!tok) continue; a.id = atoi(tok);
        tok = strtok(NULL, ","); a.customer_id = tok?atoi(tok):0;
        tok = strtok(NULL, ","); strcpy(a.description, tok?tok:"");
        tok = strtok(NULL, ","); a.price = tok?atof(tok):0;
        tok = strtok(NULL, ","); strcpy(a.status, tok?tok:"");
        tok = strtok(NULL, "\n"); strcpy(a.date, tok?tok:"");
        if (a.id == id) {
            found = 1;
            printf("Current status: %s\n", a.status);
            printf("New status (Pending, Assembled, Delivered): "); fgets(a.status, sizeof(a.status), stdin); a.status[strcspn(a.status, "\n")] = 0;
        }
        fprintf(tmp, "%d,%d,%s,%.2f,%s,%s\n", a.id, a.customer_id, a.description, a.price, a.status, a.date);
    }
    fclose(f); fclose(tmp);
    if (!found) { remove(".assemblies_tmp"); printf("Assembly ID not found.\n"); return; }
    remove(ASSEMBLIES_FILE); rename(".assemblies_tmp", ASSEMBLIES_FILE);
    printf("Assembly updated.\n");
}

/* -------------------- Reports -------------------- */
void report_low_stock() {
    if (!file_exists(PRODUCTS_FILE)) { printf("No products yet.\n"); return; }
    int threshold; printf("Low stock threshold: "); scanf("%d", &threshold); getchar();
    FILE *f = fopen(PRODUCTS_FILE, "r"); char line[MAX_LINE];
    printf("Products with stock <= %d:\n", threshold);
    printf("ID | Name | Stock\n");
    while (fgets(line, sizeof(line), f)) {
        Product p; char *tok;
        tok = strtok(line, ","); if (!tok) continue; p.id = atoi(tok);
        tok = strtok(NULL, ","); strcpy(p.name, tok?tok:"");
        tok = strtok(NULL, ","); strtok(NULL, ","); // skip category
        tok = strtok(NULL, ","); // brand
        tok = strtok(NULL, ","); // cost
        tok = strtok(NULL, ","); p.sell_price = tok?atof(tok):0;
        tok = strtok(NULL, ",\n"); p.stock = tok?atoi(tok):0;
        if (p.stock <= threshold) printf("%d | %s | %d\n", p.id, p.name, p.stock);
    }
    fclose(f);
}

void report_sales_summary() {
    if (!file_exists(SALES_FILE)) { printf("No sales recorded.\n"); return; }
    float total = 0; int count = 0;
    FILE *f = fopen(SALES_FILE, "r"); char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        char *tok = strtok(line, ","); if (!tok) continue; // id
        tok = strtok(NULL, ","); // prodid
        tok = strtok(NULL, ","); // custid
        tok = strtok(NULL, ","); // qty
        tok = strtok(NULL, ","); if (tok) { total += atof(tok); count++; }
    }
    fclose(f);
    printf("Sales Summary: %d transactions, Total revenue: %.2f\n", count, total);
}

/* -------------------- Simple Authentication -------------------- */
int ensure_default_user() {
    if (!file_exists(USERS_FILE)) {
        FILE *f = fopen(USERS_FILE, "w");
        if (!f) return 0;
        // default admin user: username=admin, password=admin
        fprintf(f, "1,admin,admin\n");
        fclose(f);
    }
    return 1;
}

int login() {
    ensure_default_user();
    char username[64], password[64];
    printf("Username: "); fgets(username, sizeof(username), stdin); username[strcspn(username, "\n")] = 0;
    printf("Password: "); fgets(password, sizeof(password), stdin); password[strcspn(password, "\n")] = 0;
    FILE *f = fopen(USERS_FILE, "r"); if (!f) return 0;
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        char *tok = strtok(line, ","); if (!tok) continue; // id
        tok = strtok(NULL, ","); if (!tok) continue; // uname
        char *uname = tok;
        tok = strtok(NULL, ",\n"); if (!tok) continue; char *pwd = tok;
        if (strcmp(uname, username) == 0 && strcmp(pwd, password) == 0) { fclose(f); return 1; }
    }
    fclose(f);
    return 0;
}

/* -------------------- Main Menu -------------------- */
void show_main_menu() {
    printf("\n========= Shop Manager =========\n");
    printf("1. Products - add/list\n");
    printf("2. Customers - add/list\n");
    printf("3. Sales - make/list\n");
    printf("4. Repairs - create/list/update\n");
    printf("5. Assemblies - create/list/update\n");
    printf("6. Reports\n");
    printf("7. Exit\n");
    printf("Select option: ");
}

int main() {
    printf("Welcome to Shop Manager (CLI)\n");
    printf("Please log in. Default admin/admin if first time.\n");
    if (!login()) { printf("Login failed. Exiting.\n"); return 1; }
    int running = 1;
    while (running) {
        show_main_menu(); int choice; if (scanf("%d", &choice) != 1) { getchar(); continue; } getchar();
        switch (choice) {
            case 1: {
                printf("Products: 1=Add 2=List\nChoice: "); int c; scanf("%d", &c); getchar(); if (c==1) add_product(); else list_products(); pause_and_wait(); break;
            }
            case 2: {
                printf("Customers: 1=Add 2=List\nChoice: "); int c; scanf("%d", &c); getchar(); if (c==1) add_customer(); else list_customers(); pause_and_wait(); break;
            }
            case 3: {
                printf("Sales: 1=Make Sale 2=List Sales\nChoice: "); int c; scanf("%d", &c); getchar(); if (c==1) make_sale(); else list_sales(); pause_and_wait(); break;
            }
            case 4: {
                printf("Repairs: 1=Create 2=List 3=Update Status\nChoice: "); int c; scanf("%d", &c); getchar(); if (c==1) create_repair_job(); else if (c==2) list_repairs(); else if (c==3) update_repair_status(); pause_and_wait(); break;
            }
            case 5: {
                printf("Assemblies: 1=Create 2=List 3=Update Status\nChoice: "); int c; scanf("%d", &c); getchar(); if (c==1) create_assembly_order(); else if (c==2) list_assemblies(); else if (c==3) update_assembly_status(); pause_and_wait(); break;
            }
            case 6: {
                printf("Reports: 1=Low Stock 2=Sales Summary\nChoice: "); int c; scanf("%d", &c); getchar(); if (c==1) report_low_stock(); else report_sales_summary(); pause_and_wait(); break;
            }
            case 7: running = 0; break;
            default: printf("Invalid choice\n"); pause_and_wait(); break;
        }
    }
    printf("Goodbye!\n");
    return 0;
}
