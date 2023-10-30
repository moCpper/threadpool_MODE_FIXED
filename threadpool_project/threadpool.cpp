#include "threadpool.h"

const int TASK_MAX_THRESHHOLD = 1024;

Threadpool::Threadpool() : 
	initThreadSize_(0),
	taskSize_(0),
	taskQueMaxThreadHold_(TASK_MAX_THRESHHOLD),
	poolMode_(PoolMode::MODE_FIXED){


}

Threadpool::~Threadpool() {}

void Threadpool::setMode(PoolMode mode) {
	poolMode_ = mode;
}

void Threadpool::setTaskQueMaxThreshHold(int threshold) {
	taskQueMaxThreadHold_ = threshold;
}

//给线程池提交任务，用户调用该接口，传入任务对象
Result Threadpool::submitTask(std::shared_ptr<Task> sp) {
	//获取锁
	std::unique_lock<std::mutex> lock(taskQueMtx_);
	//线程的通信 等待任务队列有空余,且最长阻塞不能超过1s
	if (!notFull_.wait_for(lock, std::chrono::seconds(1), [&]()->bool {
		return taskQue_.size() < (size_t)taskQueMaxThreadHold_;
		})) {
		std::cerr << "task queue is full,submit task fail," << std::endl;
		return Result(sp,false);
	}
	//如果有空余 则任务放入任务队列中
	taskQue_.emplace(sp);
	taskSize_++;
	//此时任务队列不为空，在notEmpty_上通知
	notEmpty_.notify_all();

	return Result(sp);
}

void Threadpool::start(int initThreadSize) {
	//记录初始线程个数
	initThreadSize_ = initThreadSize;
	//创建线程对象
	for (int i = 0; i < initThreadSize_; ++i) {
		auto ptr = std::make_unique<Thread>(std::bind(&Threadpool::threadFunc, this));
		threads_.emplace_back(std::move(ptr));
	}
	//启动所有线程
	for (int i = 0; i < initThreadSize_; ++i) {
		threads_[i]->start();
	}
}

//线程函数 线程池的所有线程从任务队列里消费任务
void Threadpool::threadFunc() {
	/*std::cout << "begin threadFunc tid : " << std::this_thread::get_id() << std::endl;
	std::cout << "end threadFunc" << std::this_thread::get_id() << std::endl;*/
	for (;;) {
		std::shared_ptr<Task> task;
		{
			std::unique_lock<std::mutex> lock(taskQueMtx_);

			std::cout << "tid : " << std::this_thread::get_id() << "尝试获取任务..."
				<< std::endl;

			notEmpty_.wait(lock, [&]()->bool {return taskQue_.size() > 0; });

			std::cout << "tid : " << std::this_thread::get_id() << "获取任务成功..."
				<< std::endl;

			task = taskQue_.front();
			taskQue_.pop();
			taskSize_--;

			//如果依然有其他任务，则通知其他线程执行任务
			if (taskQue_.size() > 0) {
				notEmpty_.notify_all();
			}

			notFull_.notify_all();

		}
		//当前线程执行获取的任务
		if (task != nullptr) {
			task->exec();
		}
	}
}

/* Task成员函数实现 */

Task::Task() :result_(nullptr){

}

void Task::exec() {
	if (result_ != nullptr) {
		result_->setVal(run());
	}
}

void Task::setResult(Result* res) {
	result_ = res;
}

/* 线程成员函数实现 */

Thread::Thread(ThreadFunc func) : func_(func){}

Thread::~Thread() {}

void Thread::start() {
	//创建一个线程来执行一个线程函数
	std::thread t(func_);
	t.detach();
}

/* Result成员函数实现 */

Result::Result(std::shared_ptr<Task> task, bool isVaild)
	: task_(task),isVaild_(isVaild){
	task_->setResult(this);
}

Any Result::get(){
	if (!isVaild_) {
		return "";
	}
	sem_.wait();
	return std::move(any_);
}

void Result::setVal(Any any) {
	any_ = std::move(any);
	sem_.post();
}
