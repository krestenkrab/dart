/*
 * Copyright (c) 2011, the Dart project authors.
 * 
 * Licensed under the Eclipse Public License v1.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 * 
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */
package com.google.dart.tools.core.internal.indexer.contributor;

import com.google.dart.compiler.ast.DartUnit;
import com.google.dart.indexer.exceptions.IndexRequestFailed;
import com.google.dart.indexer.exceptions.IndexRequestFailedUnchecked;
import com.google.dart.indexer.index.configuration.ContributorWrapper;
import com.google.dart.indexer.index.configuration.Processor;
import com.google.dart.indexer.index.updating.FileInfoUpdater;
import com.google.dart.indexer.workspace.index.IndexingTarget;
import com.google.dart.tools.core.DartCore;
import com.google.dart.tools.core.internal.indexer.task.CompilationUnitIndexingTarget;
import com.google.dart.tools.core.internal.util.ASTCache;
import com.google.dart.tools.core.model.CompilationUnit;
import com.google.dart.tools.core.model.DartElement;
import com.google.dart.tools.core.model.DartLibrary;
import com.google.dart.tools.core.model.DartModelException;

import org.eclipse.core.resources.IFile;

import java.util.Map;

public class DartProcessor implements Processor {
  public static final String ID = "com.google.indexer.processor.dart";

  private ContributorWrapper[] contributors;

  private ASTCache astCache = new ASTCache();

  public DartProcessor() {
    super();
  }

  @Override
  public long getAndResetTimeSpentParsing() {
    return astCache.getAndResetTimeSpentParsing();
  }

  @Override
  public void initialize(ContributorWrapper[] calculators,
      Map<String, Processor> idsToUsedProcessors) {
    this.contributors = calculators;
  }

  public void process(CompilationUnit compilationUnit, DartUnit ast, FileInfoUpdater updater)
      throws IndexRequestFailed {
    for (ContributorWrapper wrapper : contributors) {
      DartContributor contributor = (DartContributor) wrapper.getContributor();
      try {
        contributor.initialize(compilationUnit, updater.getLayerUpdater(wrapper.getLayer()));
        ast.accept(contributor);
      } catch (ThreadDeath exception) {
        throw exception;
      } catch (IndexRequestFailedUnchecked exception) {
        throw exception.unwrap();
      } catch (Throwable exception) {
        DartCore.logError("Could not use " + contributor.getClass() + " to process "
            + compilationUnit.getElementName(), exception);
      }
    }
  }

  @Override
  public void processTarget(IndexingTarget target, FileInfoUpdater updater)
      throws IndexRequestFailed {
    if (target instanceof CompilationUnitIndexingTarget) {
      CompilationUnitIndexingTarget cuTarget = (CompilationUnitIndexingTarget) target;
      CompilationUnit compilationUnit = cuTarget.getCompilationUnit();
      if (compilationUnit != null && compilationUnit.exists()) {
        DartUnit unit = cuTarget.getAST();
        if (unit == null) {
          unit = astCache.getAST(compilationUnit);
        }
        if (unit != null) {
          process(compilationUnit, unit, updater);
        }
      }
    } else {
      IFile file = target.getFile();
      String fileName = file.getName();
      if (DartCore.isDartLikeFileName(fileName)) {
        CompilationUnit compilationUnit = null;
        DartElement element = DartCore.create(file);
        if (element instanceof CompilationUnit) {
          compilationUnit = (CompilationUnit) element;
        } else if (element instanceof DartLibrary) {
          try {
            compilationUnit = ((DartLibrary) element).getDefiningCompilationUnit();
          } catch (DartModelException exception) {
            DartCore.logError(
                "Could not get defining compilation unit for " + element.getElementName(),
                exception);
          }
        }
        if (compilationUnit != null && compilationUnit.exists()) {
          DartUnit unit = astCache.getAST(compilationUnit);
          if (unit != null) {
            process(compilationUnit, unit, updater);
          }
        } else {
          // This compilation unit is not on the build path of a Dart project, so
          // we are skipping it.
        }
      }
    }
  }

  @Override
  public void transactionEnded() {
    astCache.flush();
  }

//  private void enqueueSourceFiles(IFile file) {
//    File libraryFile = file.getLocation().toFile();
//    LibrarySource librarySource = new UrlLibrarySource(libraryFile);
//    ErrorCollector collector = new ErrorCollector();
//    LibraryUnit libraryUnit = LibraryParser.parse(librarySource, collector);
//    if (libraryUnit == null) {
//      MultiStatus status = new MultiStatus(DartCore.PLUGIN_ID, 0, "Could not parse library "
//          + librarySource.getUri(), null);
//      List<DartCompilationError> errors = collector.getErrors();
//      for (DartCompilationError error : errors) {
//        status.add(new Status(IStatus.WARNING, DartCore.PLUGIN_ID, error.toString()));
//      }
//      DartCore.getPlugin().getLog().log(status);
//      return;
//    }
//    IWorkspaceRoot root = ResourcesPlugin.getWorkspace().getRoot();
//    List<IFile> changedFiles = new ArrayList<IFile>();
//    for (LibraryNode node : libraryUnit.getSourcePaths()) {
//      DartSource source = librarySource.getSourceFor(node.getText());
//      IFile[] unitFiles = ResourceUtil.getResources(source.getUri());
//      if (unitFiles != null && unitFiles.length == 1) {
//        changedFiles.add(unitFiles[0]);
//      }
//    }
//    DartModelManager.getInstance().removeLibraryInfoAndChildren(
//        libraryFile.getParentFile().getAbsoluteFile().toURI());
//    StandardDriver.getInstance().enqueueChangedFiles(
//        changedFiles.toArray(new IFile[changedFiles.size()]));
//  }
}
