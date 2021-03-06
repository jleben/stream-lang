#pragma once

#include "func_copy.hpp"
#include "name_provider.hpp"
#include "../common/functional_model.hpp"
#include "../common/func_model_visitor.hpp"
#include "../utility/context.hpp"
#include "../common/func_model_printer.hpp"

#include <unordered_set>

namespace arrp {

namespace fn = stream::functional;

using stream::context;
using fn::id_ptr;
using fn::var_ptr;
using fn::expr_ptr;
using fn::copier;
using std::shared_ptr;
using std::vector;

class func_reduction : public fn::rewriter_base
{
public:
    static bool verbose();

    func_reduction(fn::name_provider &);

    void reduce(fn::id_ptr id);

private:
    virtual void visit_local_id(const id_ptr & id) override;
    fn::expr_ptr visit_func(const shared_ptr<fn::function> &) override;
    fn::expr_ptr visit_func_app(const shared_ptr<fn::func_app> &) override;
    fn::expr_ptr visit_ref(const shared_ptr<fn::reference> &) override;
    fn::expr_ptr visit_scope(const shared_ptr<fn::scope_expr> &) override;
    fn::expr_ptr apply(shared_ptr<fn::function> f,
                       fn::expr_ptr* args,
                       int count);
    static fn::expr_ptr try_expose_function(fn::expr_ptr e);

    fn::name_provider & m_name_provider;
    std::unordered_set<id_ptr> m_visited_ids;

    fn::printer m_printer;

    int new_log_tag() { return ++m_log_tag; }
    int m_log_tag = 0;
};

}
