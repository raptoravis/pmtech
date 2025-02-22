import os
import json

default_settings = dict()
default_settings["textures_dir"] = "assets/textures/"
default_settings["models_dir"] = "assets/mesh/"


def delete_orphaned_files(build_dir, platform_data_dir):
    for root, dir, files in os.walk(build_dir):
        for file in files:
            dest_file = os.path.join(root, file)
            if dest_file.find("dependencies.json") != -1:
                depends_file = open(dest_file, "r")
                depends_json = json.loads(depends_file.read())
                depends_file.close()
                for file_dependencies in depends_json["files"]:
                    for key in file_dependencies.keys():
                        for dependency_info in file_dependencies[key]:
                            if not os.path.exists(dependency_info["name"]):
                                del_path = os.path.join(platform_data_dir, key)
                                if os.path.exists(del_path):
                                    os.remove(os.path.join(platform_data_dir, key))
                                    print("deleting " + key + " source file no longer exists")
                                    print(del_path)
                                    break


def get_build_config_setting(dir_name):
    if os.path.exists("build_config.json"):
        build_config_file = open("build_config.json", "r")
        build_config_json = json.loads(build_config_file.read())
        build_config_file.close()
        if dir_name in build_config_json:
            return build_config_json[dir_name]
    return default_settings[dir_name]


def export_config_merge(master, second):
    for key in master.keys():
        if key in second.keys():
            master[key] = export_config_merge(master[key], second[key])
    for key in second.keys():
        if key not in master.keys():
            master[key] = second[key]
    return master


def get_export_config(filename):
    export_info = dict()
    rpath = filename.replace(os.getcwd(), "")
    rpath = os.path.normpath(rpath)
    sub_dirs = rpath.split(os.sep)
    full_path = os.getcwd()
    for dir in sub_dirs:
        full_path = os.path.join(full_path, dir)
        dir_export_file = os.path.join(full_path, "_export.json")
        if os.path.exists(dir_export_file):
            file = open(dir_export_file, "r")
            file_json = file.read()
            dir_info = json.loads(file_json)
            export_info = export_config_merge(export_info, dir_info)
    # print(json.dumps(export_info, indent=4, separators=(',', ': ')))
    return export_info


def unstrict_json_safe_filename(file):
    file = file.replace("\\", '/')
    file = file.replace(":", "@")
    return file


def sanitize_filename(filename):
    sanitized_name = filename.replace("@", ":")
    sanitized_name = sanitized_name.replace('/', os.sep)
    return sanitized_name


def create_info(file):
    file = sanitize_filename(file)
    modified_time = os.path.getmtime(file)
    file = unstrict_json_safe_filename(file)
    return {"name": file, "timestamp": float(modified_time)}


def create_dependency_info(inputs, outputs):
    info = dict()
    for o in outputs:
        o = unstrict_json_safe_filename(o)
        info[o] = []
        for i in inputs:
            info[o].append(create_info(i))
    return info


def check_up_to_date(dependencies, dest_file):
    filename = os.path.join(dependencies["dir"], "dependencies.json")
    if not os.path.exists(filename):
        print("depends does not exist")
        return False

    file = open(filename)
    d_str = file.read()
    d_json = json.loads(d_str)

    file_exists = False
    for d in d_json["files"]:
        for key in d.keys():
            dependecy_file = sanitize_filename(key)
            if dest_file == dependecy_file:
                for i in d[key]:
                    file_exists = True
                    sanitized = sanitize_filename(i["name"])
                    if not os.path.exists(sanitized):
                        return False
                    if i["timestamp"] < os.path.getmtime(sanitized):
                        return False

    if not file_exists:
        return False

    return True


def write_to_file(dependencies):
    dir = dependencies["dir"]
    directory_dependencies = os.path.join(dir, "dependencies.json")
    try:
        output_d = open(directory_dependencies, 'wb+')
        output_d.write(bytes(json.dumps(dependencies, indent=4), 'UTF-8'))
        output_d.close()
    except:
        return
