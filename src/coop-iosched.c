/*
 * elevator coop
 */
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/rbtree.h>

struct coop_data {
	struct rb_node node;
	struct request *rq;
	int prio;
};
void insert_node_rb_tree(struct coop_data *new, struct rb_root *the_root)
{
        struct rb_node   **temp = &(the_root->rb_node);
        struct rb_node   *parent = NULL;
        struct coop_data *curNode;


        //printk(KERN_INFO "Inserting: %d into rbtree\n", value);
        while(*temp){
                        parent = *temp;
                        curNode = rb_entry(parent, struct coop_data, node);
                        if(curNode->prio > new->prio)
                                temp = &(*temp)->rb_left;
                        else if(curNode->prio < new->prio)
                                temp = &(*temp)->rb_right;
                        //else if(curNode->pid > new->pid)
                        //        temp = &(*temp)->rb_left;
                        else
                                temp = &(*temp)->rb_right;
        }

        rb_link_node(&new->node, parent, temp);
        rb_insert_color(&new->node, the_root);

}
static void coop_merged_requests(struct request_queue *q, struct request *rq,
				 struct request *next)
{
	//delete the next request
	//Get the root
	struct rb_root *the_root  = q->elevator->elevator_data;
	struct rb_node *tmp = NULL;
	struct coop_data *cd = NULL;

	//Do inorder search for request
	tmp = rb_first(the_root);
	if(tmp == NULL)
		return;
	cd = rb_entry(tmp, struct coop_data, node);
	while(cd->rq != next && tmp != NULL){
	       tmp = rb_next(tmp);
	       cd  = rb_entry(tmp, struct coop_data, node);
	}
	if(tmp == NULL)
		return;
	else{
		cd = rb_entry(tmp, struct coop_data, node);
		rb_erase(tmp, the_root);
		kfree(cd);
		return;
	}
}

static int coop_dispatch(struct request_queue *q, int force)
{
	//Get the elevator queue
	struct rb_root *the_root = q->elevator->elevator_data;
	struct request *rq;
	struct rb_node* first;
	struct coop_data* cd;

	//Get the next request which should be dispatched
	first = rb_first(the_root);
	if(first == NULL)
		return 0;

	cd  = rb_entry(first, struct coop_data, node);
	rq = cd->rq;
	// If there exists a request
	if (rq) {
		//remove the entry from the queue
		rb_erase(first, the_root);
		//DISPATCH
		elv_dispatch_sort(q, rq);
		return 1;
	}
	return 0;
}

static void coop_add_request(struct request_queue *q, struct request *rq)
{
	static u64 count = 0;
	static u64 vruntime_max = 0;
	struct coop_data *cd;
	struct task_struct *task = current;
	struct sched_entity se = task->se;
	u64 curr_vruntime = se.vruntime;
	//Get the elevator queue
	struct rb_root *the_root = q->elevator->elevator_data;

	//Add the request to the queue
	count++;
	if(curr_vruntime > vruntime_max)
		vruntime_max = curr_vruntime;
	cd = (struct coop_data*)kmalloc(sizeof(struct coop_data), GFP_ATOMIC);
	if(cd == NULL)
		return;
	cd->rq = rq;
	cd->prio = count + curr_vruntime/vruntime_max*128;
	insert_node_rb_tree(cd, the_root);
	return;	

}

static struct request *
coop_former_request(struct request_queue *q, struct request *rq)
{
	//Get the root
	struct rb_root *the_root  = q->elevator->elevator_data;
	struct rb_node *tmp = NULL;
	struct coop_data *cd = NULL;

	//Do inorder search for request
	tmp = rb_first(the_root);
	if(tmp == NULL)
		return NULL;
	
	cd = rb_entry(tmp, struct coop_data, node);
	while(cd->rq != rq && tmp != NULL){
	       tmp = rb_next(tmp);
	       cd = rb_entry(tmp, struct coop_data, node);
	}
	
	if(tmp == NULL)
		return NULL;
	else{
		tmp = rb_prev(tmp);
		cd = rb_entry(tmp, struct coop_data, node);
		return cd->rq;
	}

}

static struct request *
coop_latter_request(struct request_queue *q, struct request *rq)
{
	//Get the root
	struct rb_root *the_root  = q->elevator->elevator_data;
	struct rb_node *tmp = NULL;
	struct coop_data *cd = NULL;

	//Do inorder search for request
	tmp = rb_first(the_root);
	if(tmp == NULL)
		return NULL;
	cd = rb_entry(tmp, struct coop_data, node);
	while(cd->rq != rq && tmp != NULL){
	       tmp = rb_next(tmp);
	       cd = rb_entry(tmp, struct coop_data, node);
	}
	if(tmp == NULL)
		return NULL;
	else{
		tmp = rb_next(tmp);
		if(tmp == NULL)
			return NULL;
		cd = rb_entry(tmp, struct coop_data, node);
		return cd->rq;
	}
}

static int coop_init_queue(struct request_queue *q, struct elevator_type *e)
{
	//struct coop_data *cd;
	struct rb_root *the_root;
	struct elevator_queue *eq;

	//allocate a elvator and bind the request queue and 
	//elevator type to this new elevator
	eq = elevator_alloc(q, e);
	if (!eq)
		return -ENOMEM;

	//allocate a queue
	the_root = (struct rb_root*)kmalloc(sizeof(struct rb_root), GFP_KERNEL);
	if (!the_root) {
		return -ENOMEM;
	}
	//Initialize the root
	*the_root = RB_ROOT;
	//Link elevator to the queue specific to the implementation
	eq->elevator_data = the_root;


	spin_lock_irq(q->queue_lock);
	q->elevator = eq;
	spin_unlock_irq(q->queue_lock);
	return 0;
}

static void coop_exit_queue(struct elevator_queue *e)
{
	struct rb_root *the_root = e->elevator_data;
	struct rb_node* tmp;
	//struct coop_data *curr_node;
	while(1){
		tmp = rb_first(the_root);
		if(tmp == NULL)
			break;
		rb_erase(tmp, the_root);
		kfree(rb_entry(tmp, struct coop_data, node));
	}
	//kfree(cd);
}

static struct elevator_type elevator_coop = {
	.ops.sq = {
		.elevator_merge_req_fn		= coop_merged_requests,
		.elevator_dispatch_fn		= coop_dispatch,
		.elevator_add_req_fn		= coop_add_request,
		.elevator_former_req_fn		= coop_former_request,
		.elevator_latter_req_fn		= coop_latter_request,
		.elevator_init_fn		= coop_init_queue,
		.elevator_exit_fn		= coop_exit_queue,
	},
	.elevator_name = "coop",
	.elevator_owner = THIS_MODULE,
};

static int __init coop_init(void)
{
	return elv_register(&elevator_coop);
}

static void __exit coop_exit(void)
{
	elv_unregister(&elevator_coop);
}

module_init(coop_init);
module_exit(coop_exit);


MODULE_AUTHOR("Jens Axboe");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("No-op IO scheduler");
