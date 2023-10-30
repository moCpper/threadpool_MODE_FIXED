#include"threadpool.h"


class MyTask : public Task {
public:
	MyTask(int x,int y):x_(x),y_(y){}
	virtual Any run() override {
		std::cout << "begin threadFunc tid : " << std::this_thread::get_id() << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(2));
		std::cout << "end threadFunc" << std::this_thread::get_id() << std::endl;
		int tmp = 0;
		for (int i{ x_ }; i <= y_; i++) {
			tmp += i;
		}
		return tmp;
	}

private:
	int x_, y_;
};


int main(){
	Threadpool pool;
	pool.start();

	Result r1{ pool.submitTask(std::make_shared<MyTask>(10,20)) };
	Result r2{ pool.submitTask(std::make_shared<MyTask>(21,30)) };
	Result r3{ pool.submitTask(std::make_shared<MyTask>(31,40)) };

	int x1 = r1.get().cast_<int>();
	int x2 = r2.get().cast_<int>();
	int x3 = r3.get().cast_<int>();
	std::cout << x1 + x2 + x3 << std::endl;

	int s{ 0 };
	for (int i{ 10 }; i <= 40; i++) {
		s += i;
	}
	std::cout << s << std::endl;
	getchar();
	//std::this_thread::sleep_for(std::chrono::seconds(5));
}
