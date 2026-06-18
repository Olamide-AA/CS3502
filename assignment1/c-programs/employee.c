#include <stdio.h>
#include <string.h>

int main()  {
	char name[50];         // Array to hold up 49 characters + null terminator
	int employee_id;
	float hours;

	printf ("=== OwlTech Employee Registration ===\n");

	printf("Enter your name: ");
	fgets(name, sizeof(name), stdin);
	name[strcspn(name, "\n")] = '\0';	//Remove the newline character

	printf("Enter your employee ID: ");
	scanf("%d", &employee_id);

	printf("Hours worked this week; ");
	scanf("%f", &hours);

	printf("\n--- Employee Summary ---\n");
	printf("Name: %s\n", name);
	printf("ID: %d\n", employee_id);
	printf("Hours: %.2f\n", hours);

	return 0;
}
