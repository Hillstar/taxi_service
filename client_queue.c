#include "client_queue.h"

int Pop_client_info_from_queue(struct Client_queue **head, struct Client_info *client_info)
{
    if(Is_list_empty((List*)*head) == 1)
        return -1;

    struct Client_queue *temp = (*head);
    memcpy(client_info, temp->client_info, sizeof(struct Client_info));
    (*head) = (*head)->next;
    free(temp);

    return 0;
}

void Show_client_queue(struct Client_queue *head)
{
	if(Is_list_empty((List*)head) == TRUE)
		return;

	struct Client_queue *temp = head;

	printf("Clients in queue: ");
	while(temp != NULL)
	{
		printf("client_fd:%i, ", temp->client_info->fd);
		temp = temp->next;
	}
	printf("\n");
}