#!/usr/bin/env python3

import argparse
import inspect
import io
import os
import sys
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Dict, List

def fromstring_with_sourcelines(text):
    # iterparse needs a file-like object
    f = io.StringIO(text)

    try:
        # iterparse automatically attaches .sourceline
        events = ("start", "end")
        context = ET.iterparse(f, events=events)

        # Build the full tree
        for _event, elem in context:
            pass

        return context.root

    except ET.ParseError:
        # Re-raise to match ET.fromstring() behavior
        raise

write_config_h = False

def is_true(child, attr):
    if child.attrib.get(attr) in ('TRUE', 'true'):
        return True
    else:
        return False

def is_false(child, attr):
    if child.attrib.get(attr) in ('FALSE', 'false'):
        return True
    else:
        return False

def is_bool(child, attr):
    if child.attrib.get(attr) in ('TRUE', 'true', 'FALSE', 'false'):
        return True
    else:
        return False

def getLineInfo():
    frame,filename,line_number,function_name,lines,index = inspect.stack()[2]
    return f"{filename}:{line_number}"

def debug(s):
    print(f"{getLineInfo()}{s}")

#
# parse functions
#
class GWBuildParser:
    def __init__(self):
        self.level = 0

    def print_level(self):
        lstring = '    '
        s = ''
        for i in range(0, self.level):
            s += lstring
        return s

    def set_var(self, name, value):
        print(f"{self.print_level()}set({name} {value})")

    def option(self, name, type, value):
        print(f"{self.print_level()}option(option_{name} {value})")

    def print(self, l):
        print(f"{self.print_level()}{l}")

    def parse_arg(self, child):
        name = child.attrib.get('name')

    def parse_buildFiles(self, child):
        name = child.attrib.get('name').replace('-', '_')
        auto = child.attrib.get('auto')
        name_input = name + '_SOURCES'
        name_output = name + '_OUTPUT'
        for c in child:
            tag = c.tag.lower()
            if tag in ('buildmessage'):
                self.parse_buildMessage(c)
            elif tag in ('cmd'):
                self.parse_cmd(c, name_input, name_output)
            elif tag in ('input'):
                self.parse_input(c, name_input)
            elif tag in ('output'):
                self.parse_output(c, name_output)
            else:
                self.parse_unhandled(c)

    def parse_buildMessage(self, child):
        self.print(f" # todo")

    def parse_checkfunctions(self, child):
        type = child.attrib.get('type')

    def parse_checkProgs(self, child):
        for c in child:
            tag = c.tag.lower()
            if tag in ('prog'):
                self.parse_prog(c)
            else:
                self.parse_unhandled(c)

    def parse_cmd(self, child, name_input, name_output):
        tool = child.attrib.get('tool')
        checkDates = child.attrib.get('checkDates')
        deleteOutFileFirst = child.attrib.get('deleteOutFileFirst')
        args = child.text
        args = args.replace('INPUT[]', name_input)
        args = args.replace('OUTPUT[0]', name_output)
        no = '${%s}' % name_output
        self.print(f"add_custom_command(")
        self.print(f"    OUTPUT {no}")
        self.print(f"    COMMAND {self.parse_value(tool)} {self.parse_value(args)}")
        self.print(f"    COMMENT <buildMESSAGE>")
        self.print(f")")

    def parse_data(self, child):
        install = child.attrib.get('install')
        DIST = child.attrib.get('DIST')
        generated = child.attrib.get('generated')
        dist = child.attrib.get('dist')

    def parse_define(self, child):
        name = ''
        if child.attrib.get('definePrefix') != None:
            name += child.attrib.get('definePrefix')
        name += child.attrib.get('name')
        value = self.parse_value(child.attrib.get('value'))
        if is_true(child, 'quoted'):
            value = '"' + value + '"'
        self.set_var(name, value)

    def parse_dep(self, child):
        required = 'REQUIRED' if is_true(child, 'required') else ''
        version = child.attrib.get('minversion') if child.attrib.get('minversion') else ''
        self.print(f"find_package({child.attrib.get('name')} {version} {required})")
        for c in child:
            tag = c.tag.lower()
            if tag in ('variables'):
                self.parse_variables(c)
            else:
                self.parse_unhandled(c)

    def parse_dependencies(self, child):
        for c in child:
            tag = c.tag.lower()
            if tag in ('dep'):
                self.parse_dep(c)
            elif tag in ('ifvarmatches'):
                self.parse_if_var_matches(c)
            elif tag in ('ifvarhasvalues'):
                self.parse_if_var_has_values(c)
            else:
                self.parse_unhandled(c)

    def parse_files(self, child, name='SOURCES'):
        files = child.text
        match = child.attrib.get('match')
        deleteOutFileFirst = child.attrib.get('deleteOutFileFirst')
        if not match:
            self.print(f"list(APPEND {name} {files})")
        else:
            self.print(f"file(GLOB {name}_FILES {match})")
            v = '${%s_FILES}' % name
            self.print(f"list(APPEND {name} {v})")

    def parse_gwbuild(self, child):
        print("cmake_minimum_required(VERSION 3.10)")
        print("set(CMAKE_CXX_STANDARD 11)")
        print("set(CMAKE_C_STANDARD 99)")
        for c in child:
            tag = c.tag.lower()
            if tag in ('project'):
                self.parse_project(c)
            else:
                self.parse_unhandled(c)

    def parse_headers(self, child):
        dist = child.attrib.get('dist')
        install = child.attrib.get('install')

    def parse_if_var_has_values(self, child):
        name = child.attrib.get('name')
        value = child.attrib.get('value')
        self.print(f"if({name} MATCHES \"{value}\")")
        self.level += 1
        for c in child:
            tag = c.tag.lower()
            if tag in ('dep'):
                self.parse_dep(c)
        self.level -= 1
        self.print(f"endif()")

    def parse_if_var_matches(self, child):
        if is_bool(child, 'value'):
            if is_true(child, 'value'):
                self.print(f"if({child.attrib.get('name')})")
            else:
                self.print(f"if(NOT {child.attrib.get('name')})")
        else:
            self.print(f"if({child.attrib.get('name')} STREQUAL \"{child.attrib.get('value')}\")")

        self.level += 1
        for c in child:
            tag = c.tag.lower()
            if tag in ('setvar'):
                self.parse_set_var(c)
            elif tag in ('define'):
                self.parse_define(c)
            elif tag in ('dep'):
                self.parse_dep(c)
            elif tag in ('then'):
                self.parse_then_else(c)
            elif tag in ('else'):
                self.level -= 1
                self.print(f"else()")
                self.level += 1
                self.parse_then_else(c)
            elif tag in ('buildfiles'):
                self.parse_buildFiles(c)
            else:
                self.parse_unhandled(c)
        self.level -= 1
        self.print(f"endif()")

    def parse_includes(self, child):
        type = child.attrib.get('type')

    def parse_input(self, child, name='SOURCES'):
        files = child.text.strip()
        if files:
            self.print(f"list(APPEND {name} {files})")
        for c in child:
            tag = c.tag.lower()
            if tag in ('files'):
                self.parse_files(c, name)
            else:
                self.parse_unhandled(c)

    def parse_lib(self, child):
        id = child.attrib.get('id')
        name = child.attrib.get('name')
        function = child.attrib.get('function')

    def parse_libraries(self, child):
        install = child.attrib.get('install')

    def parse_option(self, child):
        name = child.attrib.get('id')
        aliases = {}
        choices = []
        default = ''
        type = child.attrib.get('type')
        for c in child:
            tag = c.tag.lower()
            if tag in ('default'):
                default = c.text
            elif tag in ('choices'):
                choices = c.text.split()
            elif tag in ('alias'):
                a_name = c.attrib.get('name')
                a_values = c.text.split()
                aliases[a_name] = a_values

        if type == 'stringlist':
            self.print(f"set(option_{name} {default} CACHE STRING \"\")")
            self.print(f"set_property(CACHE option_{name} PROPERTY STRINGS {' '.join(choices)})")
            #set(BaseName "binary" CACHE STRING "BaseName chosen by the user at CMake configure time")
            #set_property(CACHE BaseName PROPERTY STRINGS binary octal decimal hexadecimal)
        else:
            self.option(name, type, default)

    def parse_output(self, child, name='OUTPUT'):
        value = child.text.strip()
        if not value:
            value = 'dummy'
        self.set_var(name, value)

    def parse_project(self, child):
        name = child.attrib.get('name')
        version = child.attrib.get('version')
        so_current = child.attrib.get('so_current')
        so_age = child.attrib.get('so_age')
        so_revision = child.attrib.get('so_revision')
        self.set_var('SO_CURRENT', so_current)
        self.set_var('SO_AGE', so_age)
        self.set_var('SO_REVISION',so_revision)
        self.print(f"project({name} LANGUAGES CXX C VERSION {version})")
        self.set_var('project_name', '${PROJECT_NAME}')
        self.set_var('project_vmajor', '${PROJECT_VERSION_MAJOR}')
        self.set_var('project_vminor', '${PROJECT_VERSION_NINOR}')
        self.set_var('project_vpatchlevel', '${PROJECT_VERSION_PATCH}')
        if is_true(child, 'write_config_h'):
            self.write_config_h = True
        for c in child:
            tag = c.tag.lower()
            if tag in ('checkprogs'):
                self.parse_checkProgs(c)
            elif tag in ('define'):
                self.parse_define(c)
            elif tag in ('dependencies'):
                self.parse_dependencies(c)
            elif tag in ('setvar'):
                self.parse_set_var(c)
            elif tag in ('option'):
                self.parse_option(c)
            elif tag in ('subdirs'):
                l = self.parse_text(c)
                for i in l:
                    self.print(f"add_subdirectory({i})")
            elif tag in ('ifvarmatches'):
                self.parse_if_var_matches(c)
            elif tag in ('ifvarhasvalues'):
                self.parse_if_var_has_values(c)
            elif tag in ('writefile'):
                self.parse_writeFile(c)
            else:
                self.parse_unhandled(c)

    def parse_prog(self, child):
        id = child.attrib.get('id')
        cmd = child.attrib.get('cmd')
        name = '${%s}' % id
        self.print(f"find_program({id} NAMES {cmd})")
        self.print(f"message(STATUS \"found {cmd} as {name}\")")
        self.set_var(id+'_EXISTS', name)

    def parse_sources(self, child):
        pass

    def parse_set_var(self, child):
        self.set_var(child.attrib.get('name').strip(), self.parse_value(child.text.strip()))

    def parse_text(self, child):
        l = [l.strip() for l in child.text.split()]
        return l

    def parse_then_else(self, child):
        for c in child:
            tag = c.tag.lower()
            if tag in ('setvar'):
                self.parse_set_var(c)
            elif tag in ('define'):
                self.parse_define(c)
            elif tag in ('ifvarmatches'):
                self.parse_if_var_matches(c)
            elif tag in ('libraries'):
                self.parse_libraries(c)
            else:
                self.parse_unhandled(c)

    def parse_target(self, child):
        type = child.attrib.get('type')
        name = child.attrib.get('name')
        so_current = child.attrib.get('so_current')
        so_age = child.attrib.get('so_age')
        so_revision = child.attrib.get('so_revision')
        install = child.attrib.get('install')

    def parse_unhandled(self, child):
        self.print(f"# {getLineInfo()} unhandled {child.tag.lower()} {child.attrib}")

    def parse_useTargets(self, child):
        pass

    def parse_value(self, v):
        if v == None:
            return ''
        else:
            return v.replace("$(","${").replace(")","}").replace("$$","$")

    def parse_variables(self, child):
        l = child.text.split()
        if len(l) > 0:
            name = l[0]
            value = l[1]
            self.set_var(name, value)

    def parse_writeFile(self, child):
        name = child.attrib.get('name')
        install = child.attrib.get('install')
        dir = '${CMAKE_CURRENT_BINARY_DIR}'
        self.print(f"configure_file({name}.in {name})")
        if install:
            self.print(f"install(FILES {dir}/{name} DESTINATION {self.parse_value(install)})")

    def parse_xdefine(self, child):
        name = child.attrib.get('name')
        value = child.attrib.get('value')
        quoted = child.attrib.get('quoted')

    def parse_file(self, path: Path):
        text = path.read_text()
        # Quick attempt to wrap as a root element if not present
        root = fromstring_with_sourcelines(text)

        tag = root.tag.lower()
        if tag in ('gwbuild'):
            self.parse_gwbuild(root)
        else:
            self.parse_unhandled(root)

def main():
    parser = argparse.ArgumentParser(description="Generate basic CMakeLists.txt from gwbuild/BUILD-like file")
    parser.add_argument("input", help="input BUILD file (XML-like)")
    parser.add_argument("-o", "--out", default="generated_cmake", help="output directory")
    parser.add_argument("--project-name", default="GeneratedProject")
    args = parser.parse_args()

    path = Path(args.input)
    if not path.exists():
        print("Input file not found:", path)
        sys.exit(1)
    gwbuildparser = GWBuildParser()
    gwbuildparser.parse_file(path)

if __name__ == "__main__":
    main()
'''
gen_path = Path("/mnt/data/generate_cmake.py")
gen_path.write_text(generator_code)
gen_path.chmod(0o755)
print("\nWrote generator to:", gen_path)

# Run the generator to create a sample output from the provided 0BUILD
import subprocess, shlex, sys
outdir = Path("/mnt/data/generated_from_0BUILD")
cmd = f"python3 {gen_path} {build_path} -o {outdir}"
print("Running:", cmd)
proc = subprocess.run(shlex.split(cmd), capture_output=True, text=True)
print("Return code:", proc.returncode)
print("Stdout:\n", proc.stdout)
print("Stderr:\n", proc.stderr)

# Show produced top-level CMakeLists.txt
sample_top = outdir / "CMakeLists.txt"
if sample_top.exists():
    print("\n===== Generated top-level CMakeLists.txt =====")
    print(sample_top.read_text())
    # write a separate file for easy download preview
    preview = Path("/mnt/data/sample_CMakeLists_from_0BUILD.txt")
    preview.write_text(sample_top.read_text())
    print("\nWrote preview to:", preview)
else:
    print("No output generated.")
'''
