import sys
import importlib
import argparse

def get_publicsymbols(mod, allsymbols):
    return [(nm, getattr(mod, nm)) for nm in allsymbols if not (nm.startswith("__"))]

def get_funcsymbols(publicsymbols):
    return [(nm, s) for (nm, s) in publicsymbols if callable(s) and type(s) != type]

def get_subtypesymbols(publicsymbols):
    return [(nm, s) for (nm, s) in publicsymbols if (type(s) == type and not type(s).__name__.startswith("Enum") and not nm.startswith("Enum"))]

def get_membersymbols(publicsymbols):
    return [(nm, s) for (nm, s) in publicsymbols if (not type(s).__name__.startswith("Enum")) and (not type(s) == type) and (not callable(s))]

def get_enumsymbols(publicsymbols):
    return [(nm[4:], s) for (nm, s) in publicsymbols if (nm.startswith("Enum"))]


def html_header(module_name):
    print(
"""
<!DOCTYPE html>
<html lang="en">
    <head>
        <meta charset="utf-8"/>
        <title>Python API Documentation: %s</title>
        <link rel="stylesheet" href="coalpy_docs_style.css"/>
        <script>
                document.addEventListener('DOMContentLoaded', function()
                {
                    function goToAnchor(anchor)
                    {
                        var loc = document.location.toString().split('#')[0];
                        document.location = loc + '#' + anchor;
                        return false;
                    }

                    g_sidebar = document.getElementsByClassName("sidebar")[0];
                    g_sidebar_ul = g_sidebar.getElementsByTagName("ul")[0];
                    g_sidebar_lis = g_sidebar_ul.getElementsByTagName("li")
                    for (var i = 0; i < g_sidebar_lis.length; ++i)
                    {
                        var licontent = g_sidebar_lis[i].getElementsByClassName("treelink")[0];
                        if (licontent == null)
                            continue;
                        
                        var licontentDot = licontent.getElementsByClassName("treedot")[0];
                        licontentDot.parentLi = g_sidebar_lis[i];
                        licontentDot.addEventListener("click",
                        function()
                        {
                            var children = this.parentLi.getElementsByTagName("ul")[0];
                            if (children == undefined || children == null)
                                return;

                            if (children.style.display == 'none')
                            {
                                this.innerHTML = "-";
                                children.style.display = 'block';
                            }
                            else
                            {
                                this.innerHTML = "+";
                                children.style.display = 'none';
                            }
                        });
                    }

                    g_names = g_sidebar_ul.getElementsByClassName("treename");
                    for (var i = 0; i < g_names.length; ++i)
                        g_names[i].addEventListener("click", function() { goToAnchor(this.innerHTML); });
                });
        </script>
    </head>
""" % (module_name), end="")

def html_doc_bar_item_name(name, can_expand=True, is_expanded=True):
    return """<div class="treelink"><div class="treedot">%s</div><span class="treename">%s</span></div>""" % (('-' if is_expanded else '+') if can_expand else "", name)

def html_class_doc_bar(class_name, class_obj, lvl = 0):
    publicsymbols = get_publicsymbols(class_obj, dir(class_obj)) 
    methodsymbols = get_funcsymbols(publicsymbols)
    subtypesymbols = get_subtypesymbols(publicsymbols)
    hasChildren = len(subtypesymbols) > 0 or len(methodsymbols) > 0
    s = html_doc_bar_item_name(class_name, True, not hasChildren)
    if hasChildren:
        if (lvl > 0):
            s += """
            <ul>"""
        else:
            s += """
            <ul style="display:none">"""
        for (nm, symb) in methodsymbols:
            s += """
            <li>""" 
            s += html_class_doc_bar(nm, symb, lvl + 1)
            s += """
            </li>""" 
        for (nm, symb) in subtypesymbols:
            s += """
            <li>""" 
            s += html_class_doc_bar(nm, symb, lvl + 1)
            s += """
            </li>""" 
        s += """
        </ul>"""
    return s

def html_doc_bar(mod):
    print("""
        <div class="sidebar">""", end="")

    publicsymbols = get_publicsymbols(mod, dir(mod))
    funcsymbols = get_funcsymbols(publicsymbols)
    typesymbols = get_subtypesymbols(publicsymbols)
    enumsymbols = get_enumsymbols(publicsymbols)
    print("""
            <ul><li>""", end="")
    print(html_doc_bar_item_name("Functions"), end="")
    print("""
                <ul>""", end="")
    for (nm, s) in funcsymbols:
        print("""
                    <li>""", end="")
        print(html_doc_bar_item_name(nm, False), end="")
        print("""
                    </li>""", end="")
    print("""
                </ul>""", end="")

    print("""
            </li>""", end="")

    print("""
            <li>""", end="")
    print(html_doc_bar_item_name("Types"), end="")
    print("""
                <ul>""", end="")
    for (nm, s) in typesymbols:
        print("""
                    <li>""", end="")
        print(html_class_doc_bar(nm, s), end="")
        print("""
                    </li>""", end="")
    print("""
                </ul>""", end="")
    print("""
            </li>""", end="")

    print("""
            <li>""", end="")
    print(html_doc_bar_item_name("Enums"), end="")
    print("""
                <ul>""", end="")
    for (nm, s) in enumsymbols:
        print("""
                    <li>""", end="")
        print(html_doc_bar_item_name(nm, False), end="")
        print("""
                    </li>""", end="")
    print("""
                </ul>""", end="")
    print("""
            </li>""", end="")

    print("""
            </ul>""", end="")
    print("""
        </div>""", end="")

def html_member_table(members):
    if len(members) == 0:
        return ""

    out = '<table class="membertable">'
    for (nm, s) in members:
        out += "<tr>"
        out += "<td>%s</td>" % (nm)
        out += "<td>%s</td>" % (s.__doc__)
        out += "</tr>" 
    out += "</table>"
    return out
    

def html_recurse_doc(parent_name, obj_name, obj, subtitle=""):
    outs = """<div id="%s" class="h3title">%s</div>""" % (obj_name, parent_name + obj_name)
    if subtitle != "":
        outs += """<div class="subtitle">%s</div>""" % (subtitle)
    outs += "<pre>%s</pre>" % (obj.__doc__)
    outs += "<br/>"
    publicsymbols = get_publicsymbols(obj, dir(obj))
    funcsymbols = get_funcsymbols(publicsymbols)
    typesymbols = get_subtypesymbols(publicsymbols)
    enumsymbols = get_enumsymbols(publicsymbols)
    members = get_membersymbols(publicsymbols)
    new_parent_name = parent_name + obj_name + "."
    is_enum = type(obj).__name__.startswith("Enum")
    
    if is_enum:
        for (nm, s) in members:
            outs += ("<div>%s</div><br/>" % nm)
    else: outs += html_member_table(members)

    for (nm, s) in funcsymbols:
        outs += html_recurse_doc(new_parent_name, nm, s, "function") 
    for (nm, s) in typesymbols:
        outs += html_recurse_doc(new_parent_name, nm, s, "type") 
    for (nm, s) in enumsymbols:
        outs += html_recurse_doc(new_parent_name, nm, s, "enum") 
    return outs

def html_doc_content(mod):
    print("""
        <div class="content">""")
    print(html_recurse_doc("", mod.__name__, mod))
    print("""
        </div>""")
    

def html_body(mod):
    print("""
    <body>""", end="")
    html_doc_bar(mod)
    html_doc_content(mod)
    print("""
    </body>""", end="")

def html_footer():
    print("""
</html>""")

def gen_doc(module_name):
    mod = importlib.import_module(module_name)
    html_header(module_name)
    html_body(mod)
    html_footer()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog="python doctool.py",
        description = "Simple hackable tool to generate documentation in html")
    parser.add_argument("module_name")

    args = parser.parse_args()
    gen_doc(args.module_name)
