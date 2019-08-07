#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <stack>
#include <tuple>
#include <queue>
#include <random>
#include <chrono>
#include <numeric>

struct Elem {
    int id;
    int parentId;
    int weight;
    std::string name;
};

std::ostream& operator<<(std::ostream& s, const Elem& e) {
    return s << "Elem { id = " << e.id << ", parentId = " << e.parentId << ", weight = " << e.weight << ", name = " << e.name << " };";
}

template<typename It> It begin(const std::pair<It,It>& p) { return p.first; }
template<typename It> It end(const std::pair<It,It>& p) { return p.second; }

struct CompElemToParentId {
  bool operator()(const Elem& el, int pid) { return el.parentId < pid; }
  bool operator()(int pid, const Elem& el) { return el.parentId > pid; }
};

struct CompareWeight {
  bool operator()(const Elem& e1, const Elem& e2) {
      if (e1.parentId == e2.parentId) {
          if (e1.weight == e2.weight) return e1.id < e2.id;
          else return e1.weight > e2.weight;
      } else return e1.parentId > e2.parentId;
  }
};

int main()
{
    // random-ness for weight
    std::mt19937_64 eng(56486749861);
    std::uniform_int_distribution<> distr(0, 10);

    // put & sort
    const int num_root_nodes = 10000000;
    const int num_child_nodes = num_root_nodes;
    const int max_child_depth = 9;
    std::priority_queue<Elem,std::vector<Elem>,CompareWeight> elems_q;
    // create root nodes
    int id = 0;
    for (int i = 0; i < num_root_nodes; ++i) {
        elems_q.push(Elem{id, -1, distr(eng), "elem" + std::to_string(id)});
        ++id;
    }
    int min_id = 0;
    int max_id = id;
    for (int depth = 0; depth < max_child_depth; ++depth) {
        std::uniform_int_distribution<> distr1(min_id, max_id);
        for (int c = 0; c < num_child_nodes; ++c) {
            elems_q.push(Elem{id, distr1(eng), distr(eng), "elem" + std::to_string(id)});
            ++id;
        }
        min_id = max_id;
        max_id = id;
    }
    std::cout << elems_q.size() << std::endl;

    // use a nicer container
    std::vector<Elem> elems;
    elems.reserve(elems_q.size());
    while (!elems_q.empty()) {
        elems.push_back(elems_q.top());
        elems_q.pop();
    }

    // main algorithm
    auto find = [&elems](int parentId) {
        // O(log n)
        return std::equal_range(elems.cbegin(), elems.cend(), parentId, CompElemToParentId());
    };

    auto time_run = [](auto f, auto&& ...args) {
        auto start = std::chrono::high_resolution_clock::now();
        f(args...);
        auto end = std::chrono::high_resolution_clock::now();
        return end-start;
    };

    auto run_stack = [&find, &elems](auto& output) {
        using arg_t = std::pair<const Elem*, int>;
        using vec_t = std::vector<arg_t>;
        vec_t vec;
        vec.reserve(elems.size());
        std::stack<arg_t, vec_t> st(std::move(vec));
        for(const auto &i : find(-1)) st.emplace(&i,0);
        while(!st.empty()) {
            auto eli = st.top();
            st.pop();
            output.emplace_back(eli.first, eli.second);
            for(auto &i: find(eli.first->id)) st.emplace(&i, eli.second+1);
        }
    };

    auto run_rec = [&find](auto &output){
        auto print_tree = [&find, &output](auto && self, std::pair<auto, auto> els, int indent) -> void {
            for (auto &el : els) {
                output.emplace_back(&el, indent);
                self(self, find(el.id), indent+1);
            }
        };
        print_tree(print_tree, find(-1), 0);
    };


    using nanosec = std::chrono::duration<double, std::milli>;



    std::vector<nanosec> elapsed_rec, elapsed_stack;

    for (int i = 0; i < 10; ++i) {
        std::vector<std::pair<const Elem*,int>> output_rec;
        output_rec.reserve(elems.size());
        elapsed_rec.emplace_back(time_run(run_rec, output_rec));

        std::vector<std::pair<const Elem*,int>> output_stack;
        output_stack.reserve(elems.size());
        elapsed_stack.emplace_back(time_run(run_stack, output_stack));
    }

//    for (const auto &i: output1) {
//        std::cout << std::string(i.second, ' ') << *i.first << '\n';
//    }
//    for (const auto &i: output2) {
//        std::cout << std::string(i.second, ' ') << *i.first << '\n';
//    }
    auto mean = [](auto rng) { return std::accumulate(rng.begin(), rng.end(), nanosec(0)) / rng.size(); };
    auto stddev = [](const auto& v, auto m) {
        std::vector<double> diff(v.size());
        std::transform(v.begin(), v.end(), diff.begin(), [m](auto x) { return x.count() - m; });
        double sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
        return std::sqrt(sq_sum / v.size());
    };
    auto report = [&mean, &stddev](std::string name, const auto& v) -> std::tuple<double, double> {
        auto m = mean(v).count();
        auto sd = stddev(v, m);
        std::cout << "Execution "<< name << " took " << m << "ms +- " << sd << std::endl;
        return std::tie(m, sd);
    };
    auto rec = report("rec", elapsed_rec);
    auto stack = report("stack", elapsed_stack);
    std::cout << "Diff stack-rec = " << std::get<0>(stack) - std::get<0>(rec) << "ms"
              << " +- " << std::get<1>(stack) + std::get<1>(rec)
              << std::endl;
    return 0;
}
