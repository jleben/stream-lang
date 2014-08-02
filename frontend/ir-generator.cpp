#include "ir-generator.hpp"
#include "error.hpp"

#include <llvm/IR/DerivedTypes.h>


using namespace std;

namespace stream {
namespace IR {

generator::generator(llvm::Module *module, environment &env):
    m_module(module),
    m_env(env),
    m_builder(module->getContext())
{
}

void generator::generate( const symbol & sym,
                          const type_ptr & result_type,
                          const vector<type_ptr> & arg_types )
{
    llvm::Type *func_ret_type = llvm_type(result_type);

    std::vector<llvm::Type*> func_arg_types;
    for (const auto & arg_type : arg_types)
        func_arg_types.push_back( llvm_type(arg_type) );

    llvm::FunctionType * func_type =
            llvm::FunctionType::get(func_ret_type,
                                    func_arg_types,
                                    false);

    llvm::Function *func =
            llvm::Function::Create(func_type,
                                   llvm::Function::ExternalLinkage,
                                   sym.name, m_module);

    llvm::BasicBlock *bb = llvm::BasicBlock::Create(llvm_context(), "entry", func);
    m_builder.SetInsertPoint(bb);

    context::scope_holder scope(m_ctx);

    context_item_ptr item = item_for_symbol(sym);

    if (func_arg_types.size())
    {
        vector<value_ptr> func_args;

        llvm::Function::arg_iterator arg_iter;
        for (arg_iter = func->arg_begin(); arg_iter != func->arg_end(); ++arg_iter)
            func_args.emplace_back( new scalar_value(arg_iter) );

        function_item *func_item = item->as_function();
        value_ptr result = value_for_function(func_item, func_args, m_ctx.root_scope());
        m_builder.CreateRet(result->get());
    }
    else
    {
        value_item *result_item = item->as_value();
        m_builder.CreateRet(result_item->get_value()->get());
    }
}

llvm::Type *generator::llvm_type( const type_ptr & t )
{
    using namespace semantic;

    switch(t->get_tag())
    {
    case type::integer_num:
        return llvm::Type::getInt32Ty(llvm_context());
    case type::real_num:
        return llvm::Type::getDoubleTy(llvm_context());
    case type::stream:
        return llvm::PointerType::get( llvm::Type::getDoubleTy(llvm_context()), 0 );
    }

    // TODO
    assert(false);
    return nullptr;
}

context_item_ptr generator::item_for_symbol( const symbol & sym )
{
    switch(sym.type)
    {
    case symbol::expression:
    {
        context::scope_holder(m_ctx, m_ctx.root_scope());
        value_ptr val = process_block(sym.source);
        return make_shared<value_item>(val);
    }
    case symbol::function:
    {
        user_func_item *f = new user_func_item;
        f->name = sym.name;
        f->parameter_names = sym.parameter_names;
        f->expression = sym.source;
        return context_item_ptr(f);
    }
#if 0
    case symbol::builtin_unary_math:
        return make_shared<builtin_unary_func_item>();
    case symbol::builtin_binary_math:
        return make_shared<builtin_binary_func_item>();
#endif
    }
    assert(false);
    throw error("Should not happen.");
}

value_ptr generator::value_for_function(function_item *func,
                                        const vector<value_ptr> & args ,
                                        context::scope_iterator scope)
{
    if (user_func_item *user_func = dynamic_cast<user_func_item*>(func))
    {
        assert(user_func->parameter_names.size() == args.size());

        context::scope_holder func_scope(m_ctx, scope);
        for (int i = 0; i < args.size(); ++i)
            m_ctx.bind(user_func->parameter_names[i],
                       make_shared<value_item>(args[i]));

        return process_block(user_func->expression);
    }

    assert(false);
#if 0
    switch (func_type->get_tag())
    {
    case type::builtin_unary_func:
    {
        if (args.size() != 1)
            throw wrong_arg_count_error(1, args.size());
        return builtin_unary_func_type(args[0]);
    }
    case type::builtin_binary_func:
    {
        if (args.size() != 2)
            throw wrong_arg_count_error(2, args.size());
        return builtin_binary_func_type(args[0], args[1]);
    }
    case type::function:
    {
        function &f = func_type->as<function>();
        if (args.size() != f.parameters.size())
            throw wrong_arg_count_error(f.parameters.size(), args.size());

        context_type::scope_holder func_scope(m_ctx, scope);

        for (int i = 0; i < args.size(); ++i)
        {
            m_ctx.bind(f.parameters[i], args[i]);
        }

        return process_block(f.expression);
    }
    default:
        throw type_error("Callee not a function.");
    }
#endif
}

value_ptr generator::process_block( const ast::node_ptr & root )
{
    assert(root->type == ast::expression_block);
    ast::list_node *expr_block = root->as_list();
    assert(expr_block->elements.size() == 2);
    const auto & stmts = expr_block->elements[0];
    const auto & expr = expr_block->elements[1];

    if (stmts)
        process_stmt_list( stmts );

    return process_expression( expr );
}

void generator::process_stmt_list( const ast::node_ptr & root )
{
    assert(root->type == ast::statement_list || root->type == ast::program);
    ast::list_node *stmts = root->as_list();
    for ( const ast::node_ptr & stmt : stmts->elements )
    {
        process_stmt(stmt);
    }
}

void generator::process_stmt( const ast::node_ptr & root )
{
    ast::list_node *stmt = root->as_list();
    const auto & id_node = stmt->elements[0];
    const auto & params_node = stmt->elements[1];
    const auto & expr_node = stmt->elements[2];

    const string & id = id_node->as_leaf<string>()->value;

    context_item_ptr ctx_item;

    if (params_node)
    {
        vector<string> parameters;
        ast::list_node *param_list = params_node->as_list();
        for ( const auto & param : param_list->elements )
        {
            string param_name = param->as_leaf<string>()->value;
            parameters.push_back(param_name);
        }

        user_func_item *f = new user_func_item;
        f->name = id;
        f->parameter_names = parameters;
        f->expression = expr_node;

        ctx_item = context_item_ptr(f);
    }
    else
    {
        value_ptr result = process_block(expr_node);
        ctx_item = make_shared<value_item>(result);
    }

    m_ctx.bind(id, ctx_item);
}

value_ptr generator::process_expression( const ast::node_ptr & root )
{
    switch(root->type)
    {
    case ast::integer_num:
    {
        int i = root->as_leaf<int>()->value;
        llvm::Value * v = llvm::ConstantInt::get(llvm_context(), llvm::APInt(32,i,true));
        return make_shared<scalar_value>(v);
    }
    case ast::real_num:
    {
        double d = root->as_leaf<double>()->value;
        llvm::Value * v = llvm::ConstantFP::get(llvm_context(), llvm::APFloat(d));
        return make_shared<scalar_value>(v);
    }
    case ast::identifier:;
        return process_identifier(root).first;
    case ast::call_expression:
        return process_call(root);
    case ast::add:
    case ast::subtract:
    case ast::multiply:
    case ast::divide:
    case ast::lesser:
    case ast::greater:
    case ast::lesser_or_equal:
    case ast::greater_or_equal:
    case ast::equal:
    case ast::not_equal:
        return process_binop(root);
#if 0
    case ast::range:
        return process_range(root);
    case ast::hash_expression:
        return process_extent(root);
    case ast::transpose_expression:
        return process_transpose(root);
    case ast::slice_expression:
        return process_slice(root);
#endif
#if 0
    case ast::for_expression:
        return process_iteration(root);
    case ast::reduce_expression:
        return process_reduction(root);
    default:
        assert(false);
        throw source_error("Unsupported expression.", root->line);
#endif
    }
}

pair<value_ptr, generator::context::scope_iterator>
generator::process_identifier( const ast::node_ptr & root )
{
    string id = root->as_leaf<string>()->value;
    if (context::item ctx_entry = m_ctx.find(id))
    {
        context_item_ptr ctx_item = ctx_entry.value();
        value_item *val_item = ctx_item->as_value();
        return make_pair(val_item->get_value(), ctx_entry.scope());
    }
    else
    {
        environment::const_iterator it = m_env.find(id);
        if (it != m_env.end())
        {
            context_item_ptr ctx_item = item_for_symbol(it->second);
            m_ctx.root_scope()->emplace(id, ctx_item);

            value_item *val_item = ctx_item->as_value();
            return make_pair(val_item->get_value(), m_ctx.root_scope());
        }
    }
    assert(false);
    throw source_error("Name not in scope.", root->line);
}

value_ptr generator::process_call( const ast::node_ptr & root )
{
    assert(root->type == ast::call_expression);

    ast::list_node * call = root->as_list();
    const auto & func_node = call->elements[0];
    const auto & args_node = call->elements[1];

    // Get function

    context_item_ptr ctx_item;
    context::scope_iterator scope;

    string id = func_node->as_leaf<string>()->value;
    if (context::item ctx_entry = m_ctx.find(id))
    {
        ctx_item = ctx_entry.value();
        scope = ctx_entry.scope();
    }
    else
    {
        environment::const_iterator it = m_env.find(id);
        if (it != m_env.end())
        {
            ctx_item = item_for_symbol(it->second);
            scope = m_ctx.root_scope();
        }
        assert(false);
    }

    // Get args

    std::vector<value_ptr> args;
    ast::list_node * arg_list = args_node->as_list();
    for (const auto & arg_node : arg_list->elements)
    {
        args.push_back( process_expression(arg_node) );
    }

    // Process function

    return value_for_function(ctx_item->as_function(), args, scope);
}

value_ptr generator::process_binop( const ast::node_ptr & root )
{
    ast::list_node *expr = root->as_list();
    value_ptr lhs = process_expression(expr->elements[0]);
    value_ptr rhs = process_expression(expr->elements[1]);

    llvm::Type *fp_type = llvm::Type::getDoubleTy(llvm_context());
    llvm::Type *int_type = llvm::Type::getInt32Ty(llvm_context());
    if ( typeid(*lhs) == typeid(scalar_value) &&
         typeid(*rhs) == typeid(scalar_value) )
    {
        llvm::Value *lhs_ir = static_cast<scalar_value&>(*lhs).get();
        llvm::Value *rhs_ir = static_cast<scalar_value&>(*rhs).get();
        llvm::Value *result_ir;

        bool fp_op = lhs_ir->getType() == fp_type || rhs_ir->getType() == fp_type;
        if (fp_op)
        {
            if (lhs_ir->getType() != fp_type)
                lhs_ir = m_builder.CreateSIToFP(lhs_ir, fp_type);
            if (rhs_ir->getType() != fp_type)
                rhs_ir = m_builder.CreateSIToFP(rhs_ir, fp_type);
            switch(root->type)
            {
            case ast::add:
                result_ir = m_builder.CreateFAdd(lhs_ir, rhs_ir); break;
            case ast::subtract:
                result_ir = m_builder.CreateFSub(lhs_ir, rhs_ir); break;
            case ast::multiply:
                result_ir = m_builder.CreateFMul(lhs_ir, rhs_ir); break;
            case ast::divide:
                result_ir = m_builder.CreateFDiv(lhs_ir, rhs_ir); break;
            default:
                assert(false);
            }
        }
        else
        {
            switch(root->type)
            {
            case ast::add:
                result_ir = m_builder.CreateAdd(lhs_ir, rhs_ir); break;
            case ast::subtract:
                result_ir = m_builder.CreateSub(lhs_ir, rhs_ir); break;
            case ast::multiply:
                result_ir = m_builder.CreateMul(lhs_ir, rhs_ir); break;
            case ast::divide:
                result_ir = m_builder.CreateSDiv(lhs_ir, rhs_ir); break;
            default:
                assert(false);
            }
        }

        return make_shared<scalar_value>(result_ir);
    }
    else
    {
        assert(false);
        throw error("Should not happen.");
    }
}

}
}
