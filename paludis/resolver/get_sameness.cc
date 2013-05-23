/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2011, 2013 Ciaran McCreesh
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

#include <paludis/resolver/get_sameness.hh>

#include <paludis/util/log.hh>
#include <paludis/util/join.hh>

#include <paludis/package_id.hh>
#include <paludis/choice.hh>
#include <paludis/metadata_key.hh>
#include <paludis/version_spec.hh>
#include <paludis/unformatted_pretty_printer.hh>
#include <paludis/name.hh>
#include <paludis/dep_spec.hh>
#include <paludis/slot_requirement.hh>
#include <paludis/version_requirements.hh>

#include <set>
#include <algorithm>

using namespace paludis;
using namespace paludis::resolver;

namespace
{
    /* since the EAPI5 syntax for rewritten := deps in the VDB doesn't
       allow us to tell whether the dep was originally a := kind or
       a :slot= kind, normalise them all to the same thing to avoid
       spurious differences */
    struct SloppySlotStringifier
    {
        /* this is probably covered by the generic case that just
           does a stringify, but a package format could theoretically
           define a different representation, so assume the standard
           form here for compatibility with the other two that we're
           assuming */
        std::string visit(const SlotAnyAtAllLockedRequirement &) const
        {
            return ":=";
        }

        std::string visit(const SlotAnyPartialLockedRequirement &) const
        {
            return ":=";
        }

        std::string visit(const SlotUnknownRewrittenRequirement &) const
        {
            return ":=";
        }

        std::string visit(const SlotRequirement & r) const
        {
            return stringify(r);
        }
    };

    struct ComparingPrettyPrinter :
        UnformattedPrettyPrinter
    {
        using UnformattedPrettyPrinter::prettify;

        const std::string prettify(const PackageDepSpec & s) const
        {
            /* cat/pkg[foo][bar] and cat/pkg[bar][foo] are the same, and := deps
             * are weird */
            std::set<std::string> tokens;

            if (s.package_ptr())
                tokens.insert("package:" + stringify(*s.package_ptr()));

            if (s.package_name_part_ptr())
                tokens.insert("package_name_part:" + stringify(*s.package_name_part_ptr()));

            if (s.category_name_part_ptr())
                tokens.insert("category_name_part:" + stringify(*s.category_name_part_ptr()));

            if (s.version_requirements_ptr())
            {
                for (VersionRequirements::ConstIterator r(s.version_requirements_ptr()->begin()),
                        r_end(s.version_requirements_ptr()->end()) ; r != r_end ; ++r)
                    tokens.insert("version_requirement:" + stringify(s.version_requirements_mode()) +
                            stringify(r->version_operator()) + stringify(r->version_spec()));
            }

            if (s.slot_requirement_ptr())
            {
                auto r(s.slot_requirement_ptr()->maybe_original_requirement_if_rewritten());
                tokens.insert("slot_requirement:" + (r ? r : s.slot_requirement_ptr())
                    ->accept_returning<std::string>(SloppySlotStringifier()));
            }

            if (s.in_repository_ptr())
                tokens.insert("in_repository:" + stringify(*s.in_repository_ptr()));

            if (s.installable_to_repository_ptr())
                tokens.insert("installable_to_repository:" + stringify(s.installable_to_repository_ptr()->repository())
                        + "/" + stringify(s.installable_to_repository_ptr()->include_masked()));

            if (s.from_repository_ptr())
                tokens.insert("from_repository:" + stringify(*s.from_repository_ptr()));

            if (s.installed_at_path_ptr())
                tokens.insert("installed_at_path:" + stringify(*s.installed_at_path_ptr()));

            if (s.installable_to_path_ptr())
                tokens.insert("installable_to_path:" + stringify(s.installable_to_path_ptr()->path())
                        + "/" + stringify(s.installable_to_path_ptr()->include_masked()));

            if (s.additional_requirements_ptr())
            {
                for (AdditionalPackageDepSpecRequirements::ConstIterator u(s.additional_requirements_ptr()->begin()),
                        u_end(s.additional_requirements_ptr()->end()) ; u != u_end ; ++u)
                    tokens.insert("additional_requirement:" + stringify(**u));
            }

            return "PackageDepSpec(" + join(tokens.begin(), tokens.end(), ";") + ")";
        }
    };

    bool is_same_dependencies(
        const std::shared_ptr<const MetadataSpecTreeKey<DependencySpecTree> > & a,
        const std::shared_ptr<const MetadataSpecTreeKey<DependencySpecTree> > & b)
    {
        if (! a)
            return ! b;
        else if (! b)
            return false;

        auto sa(a->pretty_print_value(ComparingPrettyPrinter(), { }));
        auto sb(b->pretty_print_value(ComparingPrettyPrinter(), { }));

        if (sa != sb)
            Log::get_instance()->message("resolver.get_sameness", ll_debug, lc_context) <<
                "Not same: [" << sa << "] [" << sb << "]";

        return sa == sb;
    }
}

ExistingPackageIDAttributes
paludis::resolver::get_sameness(
        const std::shared_ptr<const PackageID> & existing_id,
        const std::shared_ptr<const PackageID> & installable_id)
{
    bool is_same_version(existing_id->version() == installable_id->version());
    bool is_same(false);
    bool is_same_metadata(false);

    if (is_same_version)
    {
        is_same = true;
        is_same_metadata = true;

        std::set<ChoiceNameWithPrefix> common;
        std::shared_ptr<const Choices> installable_choices;
        std::shared_ptr<const Choices> existing_choices;

        if (existing_id->choices_key() && installable_id->choices_key())
        {
            installable_choices = installable_id->choices_key()->parse_value();
            existing_choices = existing_id->choices_key()->parse_value();

            std::set<ChoiceNameWithPrefix> i_common, u_common;
            for (Choices::ConstIterator k(installable_choices->begin()), k_end(installable_choices->end()) ;
                    k != k_end ; ++k)
            {
                if (! (*k)->consider_added_or_changed())
                    continue;

                for (Choice::ConstIterator i((*k)->begin()), i_end((*k)->end()) ;
                        i != i_end ; ++i)
                    if (co_explicit == (*i)->origin())
                        i_common.insert((*i)->name_with_prefix());
            }

            for (Choices::ConstIterator k(existing_choices->begin()), k_end(existing_choices->end()) ;
                    k != k_end ; ++k)
            {
                if (! (*k)->consider_added_or_changed())
                    continue;

                for (Choice::ConstIterator i((*k)->begin()), i_end((*k)->end()) ;
                        i != i_end ; ++i)
                    if (co_explicit == (*i)->origin())
                        u_common.insert((*i)->name_with_prefix());
            }

            std::set_intersection(
                    i_common.begin(), i_common.end(),
                    u_common.begin(), u_common.end(),
                    std::inserter(common, common.begin()));
        }

        for (std::set<ChoiceNameWithPrefix>::const_iterator f(common.begin()), f_end(common.end()) ;
                f != f_end ; ++f)
            if (installable_choices->find_by_name_with_prefix(*f)->enabled() !=
                    existing_choices->find_by_name_with_prefix(*f)->enabled())
            {
                is_same = false;
                is_same_metadata = false;
                break;
            }
    }

    if (is_same_metadata)
    {
        is_same_metadata = is_same_metadata && is_same_dependencies(existing_id->build_dependencies_key(), installable_id->build_dependencies_key());
        is_same_metadata = is_same_metadata && is_same_dependencies(existing_id->run_dependencies_key(), installable_id->run_dependencies_key());
        is_same_metadata = is_same_metadata && is_same_dependencies(existing_id->post_dependencies_key(), installable_id->post_dependencies_key());
        is_same_metadata = is_same_metadata && is_same_dependencies(existing_id->dependencies_key(), installable_id->dependencies_key());
    }

    ExistingPackageIDAttributes attrs;
    if (is_same_version) attrs += epia_is_same_version;
    if (is_same) attrs += epia_is_same;
    if (is_same_metadata) attrs += epia_is_same_metadata;

    return attrs;
}

