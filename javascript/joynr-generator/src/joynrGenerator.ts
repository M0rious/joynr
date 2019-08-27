#! /usr/bin/env node
/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */

import yargs = require("yargs");
import path = require("path");
import { execSync } from "child_process";
import * as ts from "typescript";
import * as glob from "glob";
import * as util from "util";
import * as handlebars from "handlebars";
import * as fs from "fs";
import mkdirp = require("mkdirp");
import _ = require("lodash");

const globAsync = util.promisify(glob);

const version = require("../package.json").version;
const generatorName = `joynr-generator-standalone-${version}.jar`;
const generatorPath = path.join(__dirname, "../jars", generatorName);

async function main() {
    const argv = yargs
        .usage("Usage: $0 [options]")
        .example(`$0 -m Radio.fidl -m Test.fidl -o src-gen`, "compile 2 different .fidl files")
        .option("modelPath", {
            alias: "m",
            desc: "path to a directory with fidl files, or a single fidl file (can be supplied multiple times)"
        })
        .option("outputPath", { alias: "o", demandOption: true, desc: "output path will be created if not exist" })
        .option("js", {
            boolean: true,
            desc: "compile to js with d.ts instead of ts"
        })
        .option("includes", {
            alias: "i",
            boolean: true,
            desc: "create joynr includes"
        })
        .help()
        .wrap(yargs.terminalWidth()).argv;

    const modelPathArray: string[] = Array.isArray(argv.modelPath) ? argv.modelPath : [argv.modelPath];

    await generateTSSources(modelPathArray, argv.outputPath as string);
    if (argv.includes) {
        let files: string[] = [];
        modelPathArray.forEach(modelPath => {
            if (modelPath.endsWith(".fidl")) {
                files.push(modelPath);
            } else {
                // assume directory
                files = files.concat(glob.sync(`${modelPath}/**/*.fidl`));
            }
        });
        createJoynrIncludes(files, argv.outputPath as string);
    }
    const files = await globAsync(`${argv.outputPath}/**/*.ts`);
    if (argv.js) {
        compileToJS(files);
    }
}

async function generateTSSources(modelPaths: string | string[], outputPath: string) {
    ([] as string[]).concat(modelPaths).forEach(path => {
        const command =
            `java -jar ${generatorPath}` +
            ` -modelPath ${path} -outputPath ${outputPath}` +
            ` -generationLanguage javascript`;
        console.log(`executing command: ${command}`);
        execSync(command);
    });
    console.log(`ts generation done`);
}

/**
 * Compile JS files to TS according to
 * https://github.com/microsoft/TypeScript/wiki/Using-the-Compiler-API
 * @param fileNames list of files to be compiled
 */
function compileToJS(fileNames: string[]) {
    const compileOptions = {
        noEmitOnError: true,
        noImplicitAny: true,
        strictNullChecks: true,
        noUnusedParameters: true,
        noImplicitThis: true,
        target: ts.ScriptTarget.ES2017,
        module: ts.ModuleKind.CommonJS,
        esModuleInterop: true
    };

    const program = ts.createProgram(fileNames, compileOptions);
    const emitResult = program.emit();

    const allDiagnostics = ts.getPreEmitDiagnostics(program).concat(emitResult.diagnostics);

    allDiagnostics.forEach(diagnostic => {
        if (diagnostic.file) {
            const { line, character } = diagnostic.file.getLineAndCharacterOfPosition(diagnostic.start!);
            const message = ts.flattenDiagnosticMessageText(diagnostic.messageText, "\n");
            console.log(`${diagnostic.file.fileName} (${line + 1},${character + 1}): ${message}`);
        } else {
            console.log(`${ts.flattenDiagnosticMessageText(diagnostic.messageText, "\n")}`);
        }
    });

    const exitCode = emitResult.emitSkipped ? 1 : 0;
    console.log(`Process exiting with code '${exitCode}'.`);
    process.exit(exitCode);
}

// Setup the template file that will be used to generate the JavaScript file(s). A handler
// must be registered so Handlebars can determine if a variable is an object or not.
handlebars.registerHelper("ifObject", function(this: any, item, options) {
    if (typeof item === "object") {
        return options.fn(this);
    } else {
        return options.inverse(this);
    }
});

handlebars.registerHelper("concat", function(...args) {
    return new handlebars.SafeString(
        args
            .slice(0, -1)
            .filter(Boolean)
            .join("_")
    );
});

/**
 * Scans a directory, creating a map of data in order to generate the JavaScript file.
 *
 * @returns An object mapping bracket notation interface to file path to required file
 * @param dir The directory to begin scanning for files
 * @param relativeFromDir The file path will be calculated as a relative path
 * from this directory
 */
function createRequiresFromDir(dir: string, relativeFromDir: string): Record<string, any> {
    const files = fs.readdirSync(dir);
    const modulePaths: Record<string, any> = {};

    files.forEach(file => {
        const fullPath = path.join(dir, file);
        const currentFileStat = fs.statSync(fullPath);

        if (currentFileStat.isDirectory()) {
            const subDirModules = createRequiresFromDir(fullPath, relativeFromDir);
            const transformedSubDirModules: Record<string, any> = {};
            Object.keys(subDirModules).forEach(subModule => {
                transformedSubDirModules[subModule] = subDirModules[subModule];
            });
            modulePaths[file] = transformedSubDirModules;
        } else if (currentFileStat.isFile()) {
            const moduleName = path.basename(file, ".ts");
            const modulePath = path.relative(relativeFromDir, path.join(dir, moduleName));
            modulePaths[moduleName] = modulePath;
        }
    });

    return modulePaths;
}

function createJoynrIncludes(fidlFiles: string[], outputFolder: string, fidlFileGroup: string = "joynr-includes") {
    const templateFilePath = path.join(__dirname, "joynr-require-interface.hbs");
    const templateFile = fs.readFileSync(templateFilePath, "utf8");
    const requiresTemplate = handlebars.compile(templateFile, { noEscape: true });

    const requiresPerFile: Record<string, any> = {};

    fidlFiles.forEach(fidlFile => {
        // Gather the package name in order to construct a path, generate the method
        // names and paths and then generate the file's contents.
        const fidlFileContents = fs.readFileSync(fidlFile, "utf8")!;
        const packagePath = fidlFileContents.match(/^package\s+(.+)$/m)![1];
        const packagePathParts = packagePath.split(".");
        const outputPathSuffix = path.join.apply(path, packagePathParts);
        const outputFolderPerGroup = path.join(outputFolder, fidlFileGroup);
        const newFilename = path.join(outputFolderPerGroup, `${packagePathParts.pop()}`);

        const requires = createRequiresFromDir(
            path.join(outputFolder, "joynr", outputPathSuffix),
            outputFolderPerGroup
        );

        requiresPerFile[newFilename] = _.merge(requiresPerFile[newFilename], requires);

        try {
            mkdirp.sync(outputFolderPerGroup);
        } catch (e) {
            if (e.code !== "EEXIST") {
                throw e;
            }
        }
    });

    for (const file in requiresPerFile) {
        if (requiresPerFile.hasOwnProperty(file)) {
            const requires = requiresPerFile[file];
            fs.writeFileSync(`${file}.ts`, requiresTemplate({ requires, fileName: path.basename(file, ".ts") }));
        }
    }
}

if (!module.parent) {
    main().catch(err => {
        console.log(err);
    });
}
