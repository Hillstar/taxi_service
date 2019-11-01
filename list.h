#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

struct taxi_unit
{
	int id;
	int pos[2];
	u_short port;
};

struct taxi_list
{
	struct taxi_unit car;
	struct taxi_list *next;
};

int Is_list_empty(struct taxi_list *head)
{
	if(head == NULL)
		return 1;
	else
		return 0;
}

void Show_list(struct taxi_list *head)
{
	if(Is_list_empty(head) == 1)
		return;

	struct taxi_list *temp = head;

	while(temp != NULL)
	{
		printf("taxi id:%i, ", temp->car.id);
		temp = temp->next;
	}
	printf("\n");
}

void Delete_list(struct taxi_list **head)
{
	if(Is_list_empty(*head) == 1)
		return;

	struct taxi_list *cur = *head;
	struct taxi_list *next;

	while(cur != NULL)
	{
		next = cur->next;
		free(cur);
		cur = next;
	}
}

void Delete_by_id(struct taxi_list **head, int id)
{
	if(Is_list_empty(*head) == 1)
		return;

	if((*head)->next == NULL && (*head)->car.id == id)
	{
		free(*head);
		(*head) = NULL; 
		return;
	}

	struct taxi_list *temp = (*head)->next;
	struct taxi_list *prev = (*head);

	if((*head)->car.id == id)
	{
		free(*head);
		(*head) = temp;
	}

	while(temp != NULL)
	{
		if(temp->car.id == id)
		{
			prev->next = temp->next;
			free(temp);
		}
		temp = temp->next;
	}
}

void Push(struct taxi_list **head, struct taxi_unit car) 
{
    struct taxi_list *new_node;
    new_node = malloc(sizeof(struct taxi_list));
    new_node->car = car;
    new_node->next = (*head);
    (*head) = new_node;
}

//void Find_in_list(struct taxi_list **head, int id);


/*
int main()
{
	struct taxi_list *head = NULL;
	struct taxi_unit taxi;
	taxi.id = 1;

	printf("%i is empty\n", Is_list_empty(head));
	Push(&head, taxi); 
	taxi.id = 2;
	Push(&head, taxi); 
	//printf("%i \n", head->car.id);
	Show_list(head);

	Delete_by_id(&head,1);
	Show_list(head);

	//Delete_list(head);

	return 0;
}
*/
