#include <atomic>
#include <thread>
#include <mutex>
#include <array>
#include <list>
#include <functional>
#include <condition_variable>
#include <iostream>

using namespace std;

namespace del {
    //количество потоков
    template <unsigned ThreadCount = 8>
    //очень просто, нам на вход поступает задача и при помощи методов с++ мы уведомляем поток о прибытии задачи
    //и собственно выполняем задачу
    //выполняеи задчи до тех пор пока мы не установим флаг на done
    class DelegationModel {
        //массив потоков
        array<std::thread, ThreadCount> threads;
        //массив задач которы должен выполнять поток
        //можно конечно было создать потокобезопасную очередь но не сложилось
        list<std::function<void(void)>> queue;
        //флаг выхода из программы, т.е. потоки будут выполняться до тех пора пока устновлен этот флаг
        //сброс флага идет в методе Joinчтототам
        atomic_bool done = false;
        //штука которая сигнализирует о прибытии задачи
        condition_variable job_available_var;
        //уведомляем метод wait_all о завершении выполнения задачи
        condition_variable wait_var;
        //мютекс для захвата
        mutex wait_mutex;
        //мютекс для добавления/чтения задачи (думаю не стоит объяснять зачем)
        mutex queue_mutex;
        //чтобы подвтвердить что все оперции были выполнены
        atomic<int> t = 0;

        void Task() {
            //цикл работает пока мы не установли флаг выхода из проги
            while(!done) {
                //получаем работу
                auto res = next_job();
                //выполняем работу
                res();
                cout << "задача выполнена\n";
                //уведолмяем wait_all о завершении задачи и проверки на выход
                wait_var.notify_one();
            }
        }

        //достаем задачу из начала списка и выполняем ее
        function<void(void)> next_job() {
            function<void(void)> res;
            //хватаем мьютекс
            unique_lock<mutex> job_lock(queue_mutex);
            cout << "поток прибыл и ждет задачу\n";
            //ожидаем поступления задачи
            job_available_var.wait(job_lock, [this]() ->bool { return !queue.empty() || done; } );
            cout << "поток взял задачу\n";
            if(!done) {
                //берем функцию из самого начала
                res = queue.front();
                //удаляем эту функцию для освобождения места
                queue.pop_front();
                //инкремент счетчика выполненных задач
                t++;
            }
            else { //чтобы не было ошибки возвращаем пустую функцию
                res = []{};
            }
            return res;
        }

    public:
        DelegationModel()
        {
            for(unsigned i = 0; i < ThreadCount; ++i)
                threads[i] = thread([this] { this->Task(); });
        }

        void add_job(function<void(void)> job) {
            //создаем мьютекс
            lock_guard<std::mutex> guard(queue_mutex);
            //добавлем заадачу в конец коллекции
            queue.emplace_back(job);
            //уведомляем поток о прибытии задачи
            job_available_var.notify_one();
        }

        void join_all() {
            cout <<"мы тут\n";
            wait_all();

            //устанавливаем флаг
            done = true;
            //сообщаем всем что работа завершена и мможем выходить из ожидания
            job_available_var.notify_all();
            //джоиним все потоки чтобы не было ексепшона
            for(auto &x : threads)
                if(x.joinable())
                    x.join();
            cout << "было выполнено - " << t << " задач\n";
        }

        void wait_all() {
            if(!queue.empty()) {
                unique_lock<std::mutex> lk(wait_mutex);
                //waitим пока не поступил сигнал о завершении задачи
                //далее идет проверка, если список задач пуст, то мы выходим из метода
                //иначе мы возвращаемся в эту же точку и ждем следующего сигнала
                //и так по кругу пока список задач не пуст
                //и для этого нужен мьюеткс
                wait_var.wait(lk, [this] {
                    cout << "еще осталось " << queue.size() <<" задач"<< endl;
                    return this->queue.empty();
                });
                cout << "все задачи выполнены можем завершать работу\n";
                //разблочим мьютекс
                lk.unlock();
            }
        }
    };
}