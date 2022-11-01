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
    const unsigned num_threads = 8;
    //очень просто, нам на вход поступает задача и при помощи методов с++ мы уведомляем поток о прибытии задачи
    //и собственно выполняем задачу
    //выполняеи задчи до тех пор пока мы не установим флаг на done
    class DelegationModel {
        //массив потоков
        array<thread, num_threads> threads;
        //массив задач которы должен выполнять поток
        //можно конечно было создать потокобезопасную очередь но не сложилось
        list<function<void(void)>> list_tasks;
        //флаг выхода из программы, т.е. потоки будут выполняться до тех пора пока устновлен этот флаг
        //сброс флага идет в методе Joinчтототам
        atomic_bool done = false;
        //штука которая сигнализирует о прибытии задачи
        condition_variable job_added;
        //уведомляем метод wait_all о завершении выполнения задачи
        condition_variable task_complite;
        //мютекс для захвата (check ласт метод)
        mutex wait_mutex;
        //мютекс для добавления/чтения задачи (думаю не стоит объяснять зачем)
        mutex list_task_mutex;
        //чтобы подвтвердить что все оперции были выполнены
        atomic<int> t = 0;

        void potok() {
            //цикл работает пока мы не установли флаг выхода из проги
            while(!done) {
                //получаем работу
                auto res = get_job();
                //выполняем работу
                res();
                //cout << "задача выполнена\n";
                //уведолмяем wait_all о завершении задачи и проверки на выход
                task_complite.notify_one();
            }
        }

        //достаем задачу из начала списка и выполняем ее
        function<void(void)> get_job() {
            function<void(void)> res;
            //хватаем мьютекс
            unique_lock<mutex> job_lock(list_task_mutex);
            //cout << "поток прибыл и ждет задачу\n";
            //ожидаем поступления задачи
            job_added.wait(job_lock, [this]() ->bool { return !list_tasks.empty() || done; } );
            //cout << "поток взял задачу\n";
            if(!done) {
                //берем функцию из самого начала
                res = list_tasks.front();
                //удаляем эту функцию для освобождения места
                list_tasks.pop_front();
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
            for(int i = 0; i < num_threads; ++i) {
                threads[i] = thread([=] {potok();});
            }
        }

        void add_job(const function<void(void)>& job) {
            //создаем мьютекс
            lock_guard<std::mutex> guard(list_task_mutex);
            //добавлем заадачу в конец коллекции
            list_tasks.emplace_back(job);
            //уведомляем поток о прибытии задачи
            job_added.notify_one();
        }

        void join_all() {
            //cout <<"мы тут\n";
            if(!list_tasks.empty()) {
                unique_lock<std::mutex> lk(wait_mutex);
                //waitим пока не поступил сигнал о завершении задачи
                //далее идет проверка, если список задач пуст, то мы выходим из цикла
                //иначе мы возвращаемся в эту же точку и ждем следующего сигнала
                //и так по кругу пока список задач не пуст
                //и для этого нужен unique_lock
                task_complite.wait(lk, [this] {
                    //cout << "еще осталось " << list_tasks.size() << " задач" << endl;
                    return this->list_tasks.empty();
                });
                //cout << "все задачи выполнены можем завершать работу\n";
                //разблочим мьютекс
                lk.unlock();
            }

            //устанавливаем флаг
            done = true;
            //сообщаем всем что работа завершена и мможем выходить из ожидания
            job_added.notify_all();
            //джоиним все потоки чтобы не было ексепшона
            for(auto &x : threads)
                x.join();
            cout << "было выполнено - " << t << " задач\n";
        }
    };
}