/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2006, 2007 Ciaran McCreesh <ciaranm@ciaranm.org>
 *
 * This file is part of the Paludis package manager. Paludis is free software;
 * you can redistribute it and/or modify it under the terms of the GNU General
 * Public License version 2, as published by the Free Software Foundation.
 *
 * Paludis is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <paludis/dep_spec.hh>
#include <paludis/dep_spec_flattener.hh>
#include <paludis/environment.hh>
#include <paludis/util/visitor-impl.hh>
#include <paludis/util/private_implementation_pattern-impl.hh>
#include <libwrapiter/libwrapiter_forward_iterator.hh>
#include <list>
#include <algorithm>

/** \file
 * Implementation of dep_spec_flattener.hh.
 *
 * \ingroup grpdepspecflattener
 */

using namespace paludis;

namespace paludis
{
    /**
     * Implementation data for DepSpecFlattener.
     *
     * \ingroup grpdepspecflattener
     */
    template<>
    struct Implementation<DepSpecFlattener>
    {
        const Environment * const env;

        const tr1::shared_ptr<const PackageID> pkg;

        std::list<tr1::shared_ptr<const StringDepSpec > > specs;

        Implementation(const Environment * const e,
                const tr1::shared_ptr<const PackageID> p) :
            env(e),
            pkg(p)
        {
        }
    };
}

DepSpecFlattener::DepSpecFlattener(
        const Environment * const env,
        const tr1::shared_ptr<const PackageID> & pkg) :
    PrivateImplementationPattern<DepSpecFlattener>(new Implementation<DepSpecFlattener>(
                env, pkg))
{
}

DepSpecFlattener::~DepSpecFlattener()
{
}

DepSpecFlattener::ConstIterator
DepSpecFlattener::begin() const
{
    return ConstIterator(_imp->specs.begin());
}

DepSpecFlattener::ConstIterator
DepSpecFlattener::end() const
{
    return ConstIterator(_imp->specs.end());
}

void DepSpecFlattener::visit_sequence(const UseDepSpec & u,
        FlattenableSpecTree::ConstSequenceIterator cur,
        FlattenableSpecTree::ConstSequenceIterator e)
{
    if (_imp->env->query_use(u.flag(), *_imp->pkg) ^ u.inverse())
        std::for_each(cur, e, accept_visitor(*this));
}

void DepSpecFlattener::visit_leaf(const PlainTextDepSpec & p)
{
    _imp->specs.push_back(tr1::static_pointer_cast<const StringDepSpec>(p.clone()));
}

void DepSpecFlattener::visit_leaf(const PackageDepSpec & p)
{
    _imp->specs.push_back(tr1::static_pointer_cast<const StringDepSpec>(p.clone()));
}

void DepSpecFlattener::visit_leaf(const BlockDepSpec & p)
{
    _imp->specs.push_back(tr1::static_pointer_cast<const StringDepSpec>(p.clone()));
}

void DepSpecFlattener::visit_leaf(const URIDepSpec & p)
{
    _imp->specs.push_back(tr1::static_pointer_cast<const StringDepSpec>(p.clone()));
}

