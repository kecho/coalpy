import sys
import importlib
import argparse

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
                        licontent.parentLi = g_sidebar_lis[i];
                        licontent.dot = licontentDot == undefined ? null : licontentDot;
                        licontent.addEventListener("click",
                        function()
                        {
                            var children = this.parentLi.getElementsByTagName("ul")[0];
                            if (children == undefined || children == null)
                                return;

                            if (children.style.display == 'none')
                            {
                                if (this.dot != null) this.dot.innerHTML = "-";
                                children.style.display = 'block';
                            }
                            else
                            {
                                if (this.dot != null) this.dot.innerHTML = "+";
                                children.style.display = 'none';
                            }
                        });
                    }

                    g_names = g_sidebar_ul.getElementsByClassName("treename");
                    console.log(g_names)
                    for (var i = 0; i < g_names.length; ++i)
                        g_names[i].addEventListener("click", function() { goToAnchor(this.innerHTML); });
                });
        </script>
    </head>
""" % (module_name), end="")

def html_doc_bar_item_name(name, can_expand=True, is_expanded=True):
    return """<div class="treelink"><div class="treedot">%s</div><span class="treename">%s</span></div>""" % (('-' if is_expanded else '+') if can_expand else "", name)

def html_class_doc_bar(class_name, class_obj, lvl = 0):
    allsymbols = dir(class_obj)
    publicsymbols = [(nm, getattr(class_obj, nm)) for nm in allsymbols if not (nm.startswith("__") or nm.startswith("Enum"))]
    methodsymbols = [(nm, s) for (nm, s) in publicsymbols if callable(s) and type(s) != type]
    subtypesymbols = [(nm, s) for (nm, s) in publicsymbols if (type(s) == type and not type(s).__name__.startswith("Enum"))]
    members = [(nm, s) for (nm, s) in publicsymbols if (not type(s).__name__.startswith("Enum")) and (not type(s) == type) and (not callable(s))]
    hasChildren = len(members) > 0 or len(subtypesymbols) > 0 or len(methodsymbols) > 0
    s = html_doc_bar_item_name(class_name, True, not hasChildren)
    if hasChildren:
        if (lvl > 0):
            s += """
            <ul>"""
        else:
            s += """
            <ul style="display:none">"""
        for (nm, symb) in members:
            s += """
            <li>""" 
            s += html_class_doc_bar(nm, symb, lvl + 1)
            s += """
            </li>""" 
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

    allsymbols = dir(mod)
    publicsymbols = [(nm, getattr(mod, nm)) for nm in allsymbols if not (nm.startswith("__") or nm.startswith("Enum"))]
    funcsymbols = [(nm, s) for (nm, s) in publicsymbols if callable(s) and type(s) != type]
    typesymbols = [(nm, s) for (nm, s) in publicsymbols if (type(s) == type and not type(s).__name__.startswith("Enum"))]
    enumsymbols = [(nm, s) for (nm, s) in publicsymbols if (type(s).__name__.startswith("Enum"))]
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

def html_recurse_doc(parent_name, obj_name, obj):
    outs = """<h3 id="%s">%s</h3>""" % (obj_name, parent_name + obj_name)
    outs += "<pre>%s</pre>" % (obj.__doc__)
    outs += "<br/>"
    allsymbols = dir(obj)
    publicsymbols = [(nm, getattr(obj, nm)) for nm in allsymbols if not (nm.startswith("__") or nm.startswith("Enum"))]
    funcsymbols = [(nm, s) for (nm, s) in publicsymbols if callable(s) and type(s) != type]
    typesymbols = [(nm, s) for (nm, s) in publicsymbols if (type(s) == type and not type(s).__name__.startswith("Enum"))]
    enumsymbols = [(nm, s) for (nm, s) in publicsymbols if (type(s).__name__.startswith("Enum"))]
    members = [(nm, s) for (nm, s) in publicsymbols if (not type(s).__name__.startswith("Enum")) and (not type(s) == type) and (not callable(s))]
    new_parent_name = parent_name + obj_name + "."

    for (nm, s) in members:
        outs += ("<div>%s</div><br/>" % nm)

    for (nm, s) in funcsymbols:
        outs += html_recurse_doc(new_parent_name, nm, s) 
    for (nm, s) in typesymbols:
        outs += html_recurse_doc(new_parent_name, nm, s) 
    for (nm, s) in enumsymbols:
        outs += html_recurse_doc(new_parent_name, nm, s) 
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
