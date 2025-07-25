#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>

#define GPIO_NUM (399+33)

static int irqNumber;

static irqreturn_t gpio_irq_handler(int irq, void*)
{
	pr_info("GPIO IRQ Handler triggered for IRQ: %d\n", irq);
	return IRQ_HANDLED;

}

static int __init gpio_irq_init(void){

	int ret;

	ret = gpio_is_valid(GPIO_NUM);
	if(ret < 0){
		pr_info("Invalid GPIO \n");
		return ret;
	}
	ret = gpio_request(GPIO_NUM,"IRQ_GPIO");
	if(ret < 0){
		pr_info("GPIO Request failed, ret: %d\n",ret);
		return ret;
	}

	ret = gpio_direction_input(GPIO_NUM);
	if(ret < 0){
		pr_info("unable to set gpio dir input, ret: %d\n", ret);
		goto free_gpio;
	}

	irqNumber = gpio_to_irq(GPIO_NUM);
	if(irqNumber < 0 ){
		pr_info("unable to get irqNumber for GPIO: %d , ret :%d\n",GPIO_NUM, ret);
		ret = irqNumber;
		goto free_gpio;
	}

	/*request irq */
	ret = request_irq(irqNumber, gpio_irq_handler, IRQF_TRIGGER_RISING, "GPIOIRQ","");
	if(ret < 0){
		pr_info("Interrupt request failed for irqNumber:%d GPIO:%d, ret:%d\n",irqNumber, GPIO_NUM, ret);
	       goto free_gpio;
	}


free_gpio:
	gpio_free(GPIO_NUM);

	return ret;
};
static void __exit gpio_irq_exit(void){

	free_irq(irqNumber, NULL);
    	gpio_free(GPIO_NUM);
    	pr_info("GPIO IRQ module unloaded\n");
	return;
}

/* module entry point registration */
module_init(gpio_irq_init);
module_exit(gpio_irq_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ranjeet" );
MODULE_DESCRIPTION("gpio module interrupt handler driver");



