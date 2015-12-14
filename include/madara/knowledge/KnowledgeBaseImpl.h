#ifndef _MADARA_KNOWLEDGE_BASE_IMPL_H
#define _MADARA_KNOWLEDGE_BASE_IMPL_H

/**
 * @file KnowledgeBase.h
 * @author James Edmondson <jedmondson@gmail.com>
 *
 * This file contains the KnowledgeBase class
 */

#include <string>
#include <map>

#include <ostream>
#include <vector>

#include "madara/knowledge/CompiledExpression.h"
#include "madara/knowledge/WaitSettings.h"
#include "madara/knowledge/KnowledgeUpdateSettings.h"
#include "madara/knowledge/KnowledgeRecord.h"
#include "madara/knowledge/VariableReference.h"
#include "madara/MADARA_export.h"
#include "madara/knowledge/ThreadSafeContext.h"
#include "madara/knowledge/Files.h"
#include "madara/transport/Transport.h"
#include "madara/expression/Interpreter.h"
#include "ace/SOCK_Dgram.h"

// declare ACE class so MADARA user will not have to directly link to ACE
class ACE_SOCK_Acceptor;

namespace madara
{

  namespace knowledge
  {

    /**
     * @class KnowledgeBaseImpl
     * @brief This class provides a distributed knowledge base implementation
     **/
    class KnowledgeBaseImpl
    {
    public:
      /**
       * Constructor
       **/
      KnowledgeBaseImpl ();

      /**
       * Constructor
       * @param   host      hostname/ip of this machine
       * @param   transport transport to use for knowledge dissemination
       **/
      KnowledgeBaseImpl (const std::string & host, int transport);

      /**
       * Constructor
       * @param   host      hostname/ip of this machine
       * @param   transport transport to use for knowledge dissemination
       * @param   domain  knowledge realm we want to join
       **/
      KnowledgeBaseImpl (const std::string & host, int transport,
        const std::string & domain);

      /**
       * Constructor
       * @param   host      hostname/ip of this machine
       * @param   config    transport settings to use for dissemination
       **/
      KnowledgeBaseImpl (const std::string & host, 
        const madara::transport::TransportSettings & config);

      /**
       * Destructor
       **/
      ~KnowledgeBaseImpl ();
      
      /**
       * Locks the context to prevent updates over the network
       **/
      void lock (void);
      
      /**
       * Unlocks the context to allow updates over the network (is only
       * necessary if the context has been explicitly locked)
       **/
      void unlock (void);
      
      /**
       * Starts the transport mechanism for dissemination if it is closed
       **/
      void activate_transport (void);
      
      /**
       * Gets the number of transports
       * @return the number of transports
       **/
      size_t get_num_transports (void);

      /**
       * Closes the transport mechanism so no dissemination is possible
       **/
      void close_transport (void);
      
      /**
       * Copies variables and values from source to this context. PERFORMANCE
       * NOTES: worst case depends on size of copy_set. If empty, performance
       * is always O (n), where n is number of variables in the source context.
       * If copy_set is not empty, then performance is O (m log n) where m is
       * the number of variables in copy_set and n is number of variables in
       * the source context.
       * <br>&nbsp;<br>
       * Note that this is a deep copy due to the fact
       * that source and destination are expected to have their own thread
       * management (ref-counted variables can be problematic if shallow copy).
       *
       * @param  source    the source knowledge base to copy from
       * @param  copy_set  a map of variables that should be copied. If
       *                   empty, then everything is copied. If not empty,
       *                   only the supplied variables will be copied.
       * @param  clean_copy  if true, clear the destination context (this)
       *                     before copying.
       **/
      void copy (const KnowledgeBaseImpl & source,
        const CopySet & copy_set = CopySet (),
        bool clean_copy = false);

      /**
       * Retrieves a knowledge value
       * @param key                knowledge location
       * @param settings           settings for referring to knowledge variables
       * @return                   value at knowledge location
       **/
      madara::knowledge::KnowledgeRecord get (const std::string & key,
             const KnowledgeReferenceSettings & settings =
                     KnowledgeReferenceSettings ());
      
      /**
       * Atomically returns the value of a variable.
       * @param   variable  reference to a variable (@see get_ref)
       * @param   settings  the settings for referring to variables
       * @return         the madara::knowledge::KnowledgeRecord::Integer value for the variable
       **/
      madara::knowledge::KnowledgeRecord
        get (const VariableReference & variable,
             const KnowledgeReferenceSettings & settings =
                     KnowledgeReferenceSettings ());
      
      /**
       * Atomically returns a reference to the variable. Variable references are
       * efficient mechanisms for reference variables individually--similar to
       * speedups seen from CompiledExpression.
       * @param   key       unique identifier of the variable
       * @param settings         settings for referring to knowledge variables
       * @return            reference to the variable in the context
       **/
      VariableReference
        get_ref (const std::string & key,
             const KnowledgeReferenceSettings & settings =
                     KnowledgeReferenceSettings ());
      
      /**
       * Marks the variable reference as updated
       * @param   variable  reference to a variable (@see get_ref)
       * @param   settings  settings for applying the update
       **/
      void mark_modified (const VariableReference & variable,
          const KnowledgeUpdateSettings & settings = KnowledgeUpdateSettings());

      /**
       * Marks the variable as updated
       * @param   name      name of the variable
       * @param   settings  settings for applying the update
       **/
      void mark_modified (const std::string & name,
          const KnowledgeUpdateSettings & settings = KnowledgeUpdateSettings());
      
      /**
       * Retrieves a value at a specified index within a knowledge array
       * @param key              knowledge location
       * @param index            index within the array
       * @param settings         settings for referring to knowledge variables
       * @return                 value at knowledge location
       **/
      madara::knowledge::KnowledgeRecord retrieve_index (const std::string & key,
             size_t index,
             const KnowledgeReferenceSettings & settings =
                     KnowledgeReferenceSettings ());
      
      /**
       * Retrieves a value at a specified index within a knowledge array
       * @param   variable   reference to a variable (@see get_ref)
       * @param   index      index within the array
       * @param   settings   settings for referring to knowledge variables
       * @return             value at knowledge location
       **/
      madara::knowledge::KnowledgeRecord retrieve_index (
             const VariableReference & variable,
             size_t index,
             const KnowledgeReferenceSettings & settings =
                     KnowledgeReferenceSettings ());

      /**
       * Expands a statement using variable expansion. For example, if the
       * statement were MyKnowledge.{.id}, and .id==1, then the statement
       * would be expanded to MyKnowledge.1
       *
       * @param statement          statement to expand
       * @return                   expanded statement
       **/
      std::string expand_statement (const std::string & statement) const;

      /**
      * Retrieves a stringified list of all modified variables that are ready
      * to send over transport on next send_modifieds call
      * @return  stringified list of the modified knowledge records
      **/
      std::string debug_modifieds (void) const;

      /**
      * Gets the log level
      * @return the maximum detail level to print
      **/
      int get_log_level (void);

      /**
      * Attaches a logger to be used for printing
      * @param logger  the logger the context should lose
      **/
      void attach_logger (logger::Logger & logger) const;

      /**
      * Gets the logger used for information printing
      * @return the context's logger
      **/
      logger::Logger & get_logger (void) const;

      /**
      * Sets the log level
      * @param  level  the maximum detail level to print
      **/
      void set_log_level (int level);

#ifndef _MADARA_NO_KARL_

      /**
       * Compiles a KaRL expression into an expression tree
       *
       * @param expression         expression to compile
       * @return                   compiled, optimized expression tree
       **/
      CompiledExpression
        compile (const std::string & expression);
       
      /**
       * Evaluates an expression. Always disseminates modifications.
       *
       * @param expression      KaRL expression to evaluate
       * @return                value of expression
       **/
      madara::knowledge::KnowledgeRecord evaluate (const std::string & expression);
      
      /**
       * Evaluates an expression
       *
       * @param expression      KaRL expression to evaluate
       * @param settings        Settings for evaluating and printing
       * @return                value of expression
       **/
      madara::knowledge::KnowledgeRecord evaluate (
        const std::string & expression,
        const EvalSettings & settings);

      /**
       * Evaluates an expression
       *
       * @param expression      KaRL expression to wait on (result of compile)
       * @param settings        Settings for evaluating and printing
       * @return                value of expression
       **/
      madara::knowledge::KnowledgeRecord evaluate (
        CompiledExpression & expression,
        const EvalSettings & settings);
      
      /**
       * Evaluates a root-based tree (result of compile)
       *
       * @param root            root-based tree
       * @param settings        Settings for evaluating and printing
       * @return                value of expression
       **/
      madara::knowledge::KnowledgeRecord evaluate (
        expression::ComponentNode * root,
        const EvalSettings & settings =
          EvalSettings ());

      /**
       * Waits for an expression to be non-zero.
       * Always disseminates modifications.
       *
       * @param expression      KaRL expression to wait on
       * @return                value of expression
       **/
      madara::knowledge::KnowledgeRecord wait (const std::string & expression);
      
      /**
       * Waits for an expression to be non-zero. Provides additional settings
       * for fine-tuning the time to wait and atomic print statements.
       *
       * @param expression      KaRL expression to wait on
       * @param settings        Settings for the underlying expression
       *                        evaluation and printing
       * @return                value of expression
       **/
      madara::knowledge::KnowledgeRecord wait (const std::string & expression,
        const WaitSettings & settings);

      /**
       * Waits for an expression to be non-zero. Provides additional settings
       * for fine-tuning the time to wait and atomic print statements.
       *
       * @param expression      KaRL expression to wait on (result of compile)
       * @param settings        Settings for the underlying expression
       *                        evaluation and printing
       * @return                value of expression
       **/
      madara::knowledge::KnowledgeRecord wait (CompiledExpression & expression,
        const WaitSettings & settings);
      
      /**
       * Defines a function
       * @param  name       name of the function
       * @param  func       external function to call with this name
       **/
      void define_function (const std::string & name,
        knowledge::KnowledgeRecord (*func) (FunctionArguments &, Variables &));
      
      /**
       * Defines a named function that can distinguish the name it was called
       * with in MADARA
       * @param  name       name of the function
       * @param  func       external function to call with this name
       **/
      void define_function (const std::string & name,
        knowledge::KnowledgeRecord (*func) (const char *, FunctionArguments &, Variables &));
      
#ifdef _MADARA_JAVA_
      /**
       * Defines a named python function
       * @param  name       name of the function
       * @param  callable   external java object to call with this name
       **/
      void define_function (const std::string & name, jobject callable);
#endif
      
#ifdef _MADARA_PYTHON_CALLBACKS_
      /**
       * Defines a named python function
       * @param  name       name of the function
       * @param  callable   external python function to call with this name
       **/
      void define_function (const std::string & name, boost::python::object callable);
#endif

      /**
       * Defines a MADARA KaRL function
       * @param  name       name of the function
       * @param  expression KaRL function body       
       **/
      void define_function (const std::string & name,
        const std::string & expression);
      
      /**
       * Defines a MADARA KaRL function
       * @param  name       name of the function
       * @param  expression KaRL function body       
       **/
      void define_function (const std::string & name,
        const CompiledExpression & expression);

#endif // _MADARA_NO_KARL_

      /**
       * Read a file into the knowledge base
       * @param filename           file to read
       * @param knowledge_key      key to store the file into
       * @param settings           settings for updating/evaluating
       * @return   -1 if unsuccessful, 0 otherwise
       */
      int read_file (const std::string & knowledge_key, 
                     const std::string & filename, 
        const EvalSettings & settings =
          EvalSettings ());

      /**
       * Write a file from the knowledge base to a specified location
       * @param filename           file to write to
       * @param knowledge_key       key to read the file from
       * @return   -1 if unsuccessful, 0 otherwise
       */
      ssize_t write_file (const std::string & knowledge_key, 
                      const std::string & filename);

      /**
       * Read a policy into the knowledge base
       * @param policy_key          key to read the policy into
       * @param policy_file         file to read the policy from
       */
      int read_policy (const std::string & policy_key, 
                       const std::string & policy_file);
#ifdef _USE_CID_

      /**
       * Print all redeployment algorithm results
       **/
      void print_all_redeployment_results (std::ostream & output);

      /**
       * Run all redeployment algorithms specified in Transport Settings,
       * within the latencies.algorithm_configs variable.
       **/
      void run_all (void);
      
      /**
       * Starts a latency round
       * @return  result of operation or -1 if we are shutting down
       **/
      long start_latency (void);

      /**
       * Prints latencies associated with this process id
       * @param   output    stream to print latencies to
       **/
      void print_my_latencies (std::ostream & output);

      /**
       * Prints all latencies in the context for all processes
       * @param   output    stream to print latencies to
       **/
      void print_all_latencies (std::ostream & output);

      /**
       * Prints all summations in the context for all processes
       * @param   output    stream to print summations to
       **/
      void print_all_summations (std::ostream & output);
      
      /**
       * Votes with the lowest algorithm results current in the Transport settings.
       * This is only useful if latency collection is enabled and run_all has been
       * populated with useful redeployment algorithms before being ran. See the
       * madara::transport::TransportSettings class for all voting parameters.
       **/
      long vote (void);

#endif // _USE_CID_

      /**
       * Sets a knowledge value to a specified value
       *
       * @param key             knowledge variable location
       * @param value           value to set at location
       * @param settings        settings for applying the update
       * @return                0 if successful, -1 if key is null, and
       *                        -2 if quality isn't high enough
       **/
      int set (const std::string & key, madara::knowledge::KnowledgeRecord::Integer value, 
        const EvalSettings & settings);
      
      /**
       * Sets an index within an array to a specified value
       *
       * @param key             knowledge variable location
       * @param index           index of the location in the array
       * @param value           value to set at location
       * @param settings        settings for applying the update
       * @return                0 if successful, -1 if key is null, and
       *                        -2 if quality isn't high enough
       **/
      int set_index (const std::string & key,
        size_t index,
        knowledge::KnowledgeRecord::Integer value,
        const EvalSettings & settings =
          EvalSettings ());

      /**
       * Sets a knowledge value to a specified value
       *
       * @param key             knowledge variable location
       * @param value           array of integers to set to this location
       * @param size            number of elements in the array
       * @param settings        settings for applying the update
       * @return                0 if successful, -1 if key is null, and
       *                        -2 if quality isn't high enough
       **/
      int set (const std::string & key,
        const knowledge::KnowledgeRecord::Integer * value,
        uint32_t size,
        const EvalSettings & settings = EvalSettings ());
      
      /**
       * Sets a knowledge value to a specified value
       *
       * @param key             knowledge variable location
       * @param value           array of integers to set to this location
       * @param settings        settings for applying the update
       * @return                0 if successful, -1 if key is null, and
       *                        -2 if quality isn't high enough
       **/
      int set (const std::string & key,
        const std::vector <KnowledgeRecord::Integer> & value,
        const EvalSettings & settings = EvalSettings ());
      
      /**
       * Sets a knowledge value to a specified value
       *
       * @param key             knowledge variable location
       * @param value           value to set at location
       * @param settings        settings for applying the update
       * @return                0 if successful, -1 if key is null, and
       *                        -2 if quality isn't high enough
       **/
      int set (const std::string & key, double value, 
        const EvalSettings & settings);
      
      /**
       * Sets an index within an array to a specified value
       *
       * @param key             knowledge variable location
       * @param index           index of the location in the array
       * @param value           value to set at location
       * @param settings        settings for applying the update
       * @return                0 if successful, -1 if key is null, and
       *                        -2 if quality isn't high enough
       **/
      int set_index (const std::string & key,
        size_t index,
        double value,
        const EvalSettings & settings =
          EvalSettings ());
      
      /**
       * Atomically reads a file into a variable
       * @param   variable  reference to a variable (@see get_ref)
       * @param   filename  file to read
       * @param   settings  settings for applying the update
       * @return   0 if the value was set. -1 if unsuccessful
       **/
      int
      read_file (
        const VariableReference & variable,
        const std::string & filename,
        const EvalSettings & settings =
          EvalSettings ());
      
      /**
       * Atomically sets the value of a variable to a string
       * @param   variable  reference to a variable (@see get_ref)
       * @param   value     new value of the variable
       * @param   settings  settings for applying the update
       * @return   0 if the value was set. -1 if null key
       **/
      int set (const VariableReference & variable,
        const std::string & value,
        const EvalSettings & settings =
          EvalSettings ());
      
      /**
       * Atomically sets the value of a variable to a double array.
       * @param   variable  reference to a variable (@see get_ref)
       * @param   value     a STL vector of doubles
       * @param   settings  settings for applying the update
       * @return   0 if the value was set. -1 if null key
       **/
      int set (const VariableReference & variable,
        const std::vector <double> & value,
        const EvalSettings & settings =
          EvalSettings ());
      
      /**
       * Atomically sets the value of a variable to a double array.
       * @param   variable  reference to a variable (@see get_ref)
       * @param   value     an array of doubles
       * @param   size      number of elements in the array
       * @param   settings  settings for applying the update
       * @return   0 if the value was set. -1 if null key
       **/
      int set (const VariableReference & variable,
        const double * value,
        uint32_t size,
        const EvalSettings & settings =
          EvalSettings ());
      
      /**
       * Atomically sets the value of an array index to a double.
       * @param   variable  reference to a variable (@see get_ref)
       * @param   index     index within array
       * @param   value     new value of the array index
       * @param   settings  settings for applying the update
       * @return   0 if the value was set. -1 if null key
       **/
      int set_index (const VariableReference & variable,
        size_t index, double value,
        const EvalSettings & settings =
          EvalSettings ());
      
      /**
       * Atomically sets the value of a variable to a double.
       * @param   variable  reference to a variable (@see get_ref)
       * @param   value     new value of the variable
       * @param   settings  settings for applying the update
       * @return   0 if the value was set. -1 if null key
       **/
      int set (const VariableReference & variable,
        double value,
        const EvalSettings & settings =
          EvalSettings ());
 
      /**
       * Atomically sets the value of a variable to an integer array.
       * @param   variable  reference to a variable (@see get_ref)
       * @param   value     a STL vector of Integers
       * @param   settings  settings for applying the update
       * @return   0 if the value was set. -1 if null key
       **/
      int set (const VariableReference & variable,
        const std::vector <KnowledgeRecord::Integer> & value,
        const EvalSettings & settings =
          EvalSettings ());
      
      /**
       * Atomically sets the value of a variable to an integer array.
       * @param   variable  reference to a variable (@see get_ref)
       * @param   value     an array of Integers
       * @param   size      number of elements in the array
       * @param   settings  settings for applying the update
       * @return   0 if the value was set. -1 if null key
       **/
      int set (const VariableReference & variable,
        const madara::knowledge::KnowledgeRecord::Integer * value,
        uint32_t size,
        const EvalSettings & settings =
          EvalSettings ());

      /**
       * Atomically sets the value of an array index to a double.
       * @param   variable  reference to a variable (@see get_ref)
       * @param   index     index within array
       * @param   value     new value of the array index
       * @param   settings  settings for applying the update
       * @return   0 if the value was set. -1 if null key
       **/
      int set_index (const VariableReference & variable,
        size_t index, knowledge::KnowledgeRecord::Integer value,
        const EvalSettings & settings =
          EvalSettings ());
      
      /**
       * Atomically sets the value of a variable to an integer.
       * @param   variable  reference to a variable (@see get_ref)
       * @param   value     new value of the variable
       * @param   settings  settings for applying the update
       * @return   0 if the value was set. -1 if null key
       **/
      int set (const VariableReference & variable,
        madara::knowledge::KnowledgeRecord::Integer value, 
        const EvalSettings & settings =
          EvalSettings ());
      
      /**
       * Atomically sets the value of a variable to a text file's contents.
       * @param   variable  reference to a variable (@see get_ref)
       * @param   value     new value of the variable
       * @param   size      indicates the size of the value buffer
       * @param   settings  settings for applying the update
       * @return   0 if the value was set. -1 if null key
       **/
      int set_text (const VariableReference & variable,
        const char * value, size_t size, 
        const EvalSettings & settings =
          EvalSettings ());
      
      /**
       * Atomically sets the value of a variable to the contents of a file.
       * @param   variable  reference to a variable (@see get_ref)
       * @param   value     new value of the variable
       * @param   size      indicates the size of the value buffer
       * @param   settings  settings for applying the update
       * @return   0 if the value was set. -1 if null key
       **/
      int set_file (const VariableReference & variable,
        const unsigned char * value, size_t size, 
        const EvalSettings & settings =
          EvalSettings ());
      
      /**
       * Atomically sets the value of a variable to a JPEG image
       * @param   variable  reference to a variable (@see get_ref)
       * @param   value     new value of the variable
       * @param   size      indicates the size of the value buffer
       * @param   settings  settings for applying the update
       * @return   0 if the value was set. -1 if null key
       **/
      int set_jpeg (const VariableReference & variable,
        const unsigned char * value, size_t size, 
        const EvalSettings & settings =
          EvalSettings ());
      
      /**
       * Atomically sets the value of a variable to an XML string.
       * @param   variable  reference to a variable (@see get_ref)
       * @param   value     new value of the variable
       * @param   size      indicates the size of the value buffer
       * @param   settings  settings for applying the update
       * @return   0 if the value was set. -1 if null key
       **/
      int set_xml (const VariableReference & variable,
        const char * value, size_t size, 
        const EvalSettings & settings =
          EvalSettings ());
      
      /**
       * Sets a knowledge value to a specified value
       *
       * @param key             knowledge variable location
       * @param value           array of doubles to set to this location
       * @param size            number of elements in the array
       * @param settings        settings for applying the update
       * @return                0 if successful, -1 if key is null, and
       *                        -2 if quality isn't high enough
       **/
      int set (const std::string & key,
        const double * value,
        uint32_t size,
        const EvalSettings & settings = EvalSettings ());
      
      /**
       * Sets a knowledge value to a specified value
       *
       * @param key             knowledge variable location
       * @param value           array of doubles to set to this location
       * @param settings        settings for applying the update
       * @return                0 if successful, -1 if key is null, and
       *                        -2 if quality isn't high enough
       **/
      int set (const std::string & key,
        const std::vector <double> & value,
        const EvalSettings & settings = EvalSettings ());
      
      /**
       * Sets a knowledge value to a specified value
       *
       * @param key             knowledge variable location
       * @param value           value to set at location
       * @param settings        settings for applying the update
       * @return                0 if successful, -1 if key is null, and
       *                        -2 if quality isn't high enough
       **/
      int set (const std::string & key, const std::string & value, 
        const EvalSettings & settings);

      /**
       * Returns a non-const reference to the Transport Settings
       * @return  settings reference
       **/
      madara::transport::TransportSettings & transport_settings (void);

      /**
       * Applies current time and modified to all global variables and tries
       * to send them.
       * @param settings        Settings for evaluating and printing
       * @return 0 if there was not a problem.
       **/
      int apply_modified (const EvalSettings & settings =
          EvalSettings ());

      /**
       * Checks if a knowledge location exists in the context
       *
       * @param key             knowledge variable location
       * @param settings        settings for referring to knowledge variables
       * @return                true if location has been set
       **/
      bool exists (const std::string & key,
        const KnowledgeReferenceSettings & settings =
          KnowledgeReferenceSettings ()) const;
      
      /**
       * Checks if a knowledge variable exists in the context
       *
       * @param variable        knowledge variable reference
       * @param settings        settings for referring to knowledge variables
       * @return                true if location has been set
       **/
      bool exists (const VariableReference & variable,
        const KnowledgeReferenceSettings & settings =
          KnowledgeReferenceSettings (false)) const;

      /**
       * Sets the quality of writing to a certain variable from this entity
       *
       * @param key             knowledge variable location
       * @param quality         quality of writing to this location
       * @param settings        settings for referring to knowledge variables
       **/
      void set_quality (const std::string & key, uint32_t quality,
             const KnowledgeReferenceSettings & settings =
               KnowledgeReferenceSettings ());

      /**
       * Prints all knowledge variables and values in the context
       * @param   level    level to log the knowledge at
       **/
      void print (unsigned int level) const;

      /**
       * Expands and prints a user provided statement at a logging level.
       * For example, if the statement were MyKnowledge.{.id}, and .id==1,
       * then the statement would be expanded to MyKnowledge.1
       * 
       * @param   statement  statement to expand with variable values
       * @param   level      level to log the knowledge at
       **/
      void print (const std::string & statement, unsigned int level) const;
      
      /**
       * Saves all keys and values into a string, using the underlying
       * knowledge::KnowledgeRecord::to_string function. This is an optimized
       * version that allows the specification of a target string to
       * avoid string copying (which can be expensive with longer
       * strings across multiple function boundaries). This function differs
       * from knowledge::KnowledgeRecord to_string in that it is intended to save the
       * database in a format that can be easily parseable. Consequently,
       * strings are delineated in this function by being included in single
       * quotes. Arrays are delineated with array indices [].
       * 
       * This is not appropriate
       * for saving the context if it has binary data inside of it, as
       * only the size of the data entry would be saved.
       * @param target  where to store the resulting string operation.
       * @param array_delimiter  for any arrays, the characters in between
       *                         data elements.
       * @param record_delimiter the characters to place between individual
       *                         key/value pairs
       * @param key_val_delimiter the characters to place between keys and
       *                          values.
       **/
      void to_string (std::string & target,
        const std::string & array_delimiter = ",",
        const std::string & record_delimiter = ";\n",
        const std::string & key_val_delimiter = "=") const;

      /**
      * Clears the knowledge base
      * @param  erase   If true, completely erase keys.
      *                 If false, reset knowledge
      **/
      void clear (bool erase = false);

      /**
       * Clears the knowledge base
       **/
      void clear_map (void);

      /**
       * Clears the permanent knowledge rules (unimplemented)
       **/
      void clear_rules (void);

      /**
       * Acquires the recursive lock on the knowledge base. This will
       * block any other thread from updating or using the knowledge
       * base until you call @ release.
       **/
      void acquire (void);

      /**
       * Releases a recursive lock on the knowledge base. This will
       * allow other thread to access the knowledge base if you had
       * previously called @ acquire.
       **/
      void release (void);
      
      /**
       * Attaches a transport to the Knowledge Engine. Note that the
       * transport should use the same ThreadSafeContext as the
       * Knowledge Engine.
       * @param  transport   a new transport to attach tot he Knowledge Base
       * @return             the number of transports now attached
       **/
      size_t attach_transport (madara::transport::Base * transport);
      
      /**
       * Adds a built-in transport with the specified settings
       * @param  id          unique identifier for this agent (empty string
       *                     will use default generated id)
       * @param  settings    settings for the new transport
       * @return             the number of transports now attached
       **/
      size_t attach_transport (const std::string & id,
        transport::TransportSettings & settings);
      
      /**
       * Removes a transport
       * @param   index      index of the transport to remove. If invalid,
       *                     nothing is removed.
       * @return             the size of transports now attached
       **/
      size_t remove_transport (size_t index);

      /**
       * Returns the ThreadSafeContext associated with this Knowledge
       * Base. This is necessary for creating custom transports.
       *
       * @return             the context used by the knowledge base
       **/
      ThreadSafeContext & get_context (void);

      /**
       * Returns the unique host and ephemeral binding for this Knowlede Base
       * @return             host:port identifier for this knowledge base
       **/
      std::string get_id (void);
      
      /**
       * Fills a vector with Knowledge Records that begin with a common subject
       * and have a finite range of integer values.
       * @param   subject     The common subject of the variable names. For
       *                      instance, if we are looking for a range of vars
       *                      like "var0", "var1", "var2", then the common
       *                      subject would be "var".
       * @param   start       An inclusive start index
       * @param   end         An inclusive end index
       * @param   target      The vector that will be filled with
       *                      Knowledge Record instances within the subject
       *                      range.
       * @return              entries in the resulting vector
       **/
      size_t to_vector (const std::string & subject,
                              unsigned int start,
                              unsigned int end,
                              std::vector <KnowledgeRecord> & target);
      
      /**
       * Fills a variable map with Knowledge Records that match an expression.
       * At the moment, this expression must be of the form "subject*"
       * @param   subject     An expression that matches the variable names
       *                      that are of interest. Wildcards may only be
       *                      at the end.
       * @param   target      The map that will be filled with variable names
       *                      and the Knowledge Records they correspond to
       * @return              entries in the resulting map
       **/
      size_t to_map    (const std::string & subject,
                       std::map <std::string, knowledge::KnowledgeRecord> & target);

      /**
      * Fills a variable map with list of keys according to a matching prefix,
      * suffix, and delimiter hierarchy. This is useful for understanding the
      * logical hierarchy of your variables (and also a key utility of
      * containers like @see containers::FlexMap).
      * @param   prefix      Text that must be present at the front of the key
      * @param   delimiter   Text that signifies a logical boundary in hierarchy If
      *                      empty, no check is performed.
      * @param   suffix      Text that must be present at the end of the key. If
      *                      empty, no check is performed.
      * @param   next_keys   The immediate keys in the hierarchy after prefix
      * @param   result      The map that will be filled with full variable names
      *                      and the Knowledge Records that correspond to
      * @param   just_keys   if true, do not fill result, only next_keys.
      * @return              entries in the resulting map
      **/
      size_t  to_map (const std::string & prefix,
        const std::string & delimiter,
        const std::string & suffix,
        std::vector <std::string> & next_keys,
        std::map <std::string, knowledge::KnowledgeRecord> & result,
        bool just_keys);

      /**
       * Creates a variable map with Knowledge Records that begin with the given
       * prefix. Runs in O(log n + m) time, where n is the size of the
       * KnowledgeBase, and m is the number of matching records
       *
       * @param   prefix      Prefix string to match with
       * @return              A new map with just entries starting with prefix
       **/
      knowledge::KnowledgeMap to_map (const std::string & prefix) const;

      /**
       * Saves the context to a file
       * @param   filename    name of the file to open
       * @return  total bytes written
       **/
      int64_t save_context (const std::string & filename) const;

      /**
      * Saves the context to a file as karl assignments, rather than binary
      * @param   filename    name of the file to save to
      * @return  total bytes written
      **/
      int64_t save_as_karl (const std::string & filename) const;

      /**
       * Saves a checkpoint of a list of changes to a file
       * @param   filename    name of the file to open
       * @param   reset_modifieds  if true, resets the modified list to empty.
       * @return  total bytes written
       **/

      int64_t save_checkpoint (const std::string & filename,
        bool reset_modifieds = true);
      
      /**
       * Loads the context from a file
       * @param   filename    name of the file to open
       * @param   use_id      if true, sets the unique identifier to the
       *                      one found in the saved context. If false,
       *                      keeps the default identifier.
       * @param   settings    settings for modifying context
       * @return  total bytes read
       **/
      int64_t load_context (const std::string & filename,
        bool use_id,
        const KnowledgeUpdateSettings & settings = 
              KnowledgeUpdateSettings (true, true, true, false));
      
      /**
       * Clear all modifications to the knowledge base. This action may
       * be useful if you are wanting to keep local changes but not
       * inform other agents (possibly due to a need to further process
       * and verify the information). The knowledge stays in its current
       * form (i.e., this does not roll back state or anything like that)
       */
      void clear_modifieds (void);

      /**
       * Sends all modified variables through the attached transports.
       * @param   prefix      for logging purposes, the descriptor prefix for 
       *                      calling method
       * @param   settings    settings for sending modifications
       * @return  number of transports the modifications were sent to
       **/
      int send_modifieds (
        const std::string & prefix,
        const EvalSettings & settings =
          EvalSettings ());
      
      /**
       * Wait for a change to happen to the context (e.g., from transports)
       **/
      void wait_for_change (void);
      
      /**
       * Binds to an ephemeral port for unique tie breakers in global ordering.
       * Call this function if you want to generate a unique id for a
       * custom transport to use (or a built-in transport if you are creating
       * one outside of the KnowledgeBase.
       * @param  host   unique host identifier string ("" if requesting a
       *                generated id.
       * @return        unique host id (same as host param if host is not "")
       **/
      std::string setup_unique_hostport (const std::string & host = "");

    private:
      ThreadSafeContext                 map_;
      ACE_SOCK_Dgram                      unique_bind_;
      std::string                         id_;
      transport::QoSTransportSettings   settings_;
      Files                               files_;

      madara::transport::Transports transports_;
    };
  }
}
    
// include the inline functions
#include "KnowledgeBaseImpl.inl"

#endif  // _MADARA_KNOWLEDGE_BASE_IMPL_H