#ifndef _DEQUE_HPP_
#define _DEQUE_HPP_

#include <vector>

template <typename A>
class Deque{
    size_t frontSize = 0;
    std::vector<A> back;   // dirction: to the right, excluding index 0
    std::vector<A> front;  // direction: to the left

    public:
    void push_front(A& a){
        front.push_back(a);
        frontSize++;
    }
    void push_back(A& a){
        back.push_back(a);
    }
    void push_front(const A a){
        front.push_back(a);
        frontSize++;
    }
    void push_back(const A a){
        back.push_back(a);
    }
    // void pop_front(){
    //     front.pop_back();
    //     frontSize--;
    // }
    // void pop_back(){
    //     back.pop_back();
    //     backSize--;
    // }
    A& at(size_t i){
        if (i >= frontSize){
            return back.at(i-frontSize);
        }else{
            return front.at(frontSize-1-i);
        }
    }
    A& operator[](size_t i){
        if (i >= frontSize){
            return back[i-frontSize];
        }else{
            return front[frontSize-1-i];
        }
    }
};

#endif
