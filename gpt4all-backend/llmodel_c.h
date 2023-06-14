#ifndef LLMODEL_C_H
#define LLMODEL_C_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __GNUC__
#define DEPRECATED __attribute__ ((deprecated))
#elif defined(_MSC_VER)
#define DEPRECATED __declspec(deprecated)
#else
#pragma message("WARNING: You need to implement DEPRECATED for this compiler")
#define DEPRECATED
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opaque pointer to the underlying model.
 */
typedef void *llmodel_model;

/**
 * Structure containing any errors that may eventually occur
 */
struct llmodel_error {
    const char *message;  // Human readable error description; Thread-local; guaranteed to survive until next llmodel C API call
    int code;             // errno; 0 if none
};
#ifndef __cplusplus
typedef struct llmodel_error llmodel_error;
#endif

/**
 * llmodel_prompt_context structure for holding the prompt context.
 * NOTE: The implementation takes care of all the memory handling of the raw logits pointer and the
 * raw tokens pointer. Attempting to resize them or modify them in any way can lead to undefined
 * behavior.
 */
struct llmodel_prompt_context {
    float *logits;          // logits of current context
    size_t logits_size;     // the size of the raw logits vector
    int32_t *tokens;        // current tokens in the context window
    size_t tokens_size;     // the size of the raw tokens vector
    int32_t n_past;         // number of tokens in past conversation
    int32_t n_ctx;          // number of tokens possible in context window
    int32_t n_predict;      // number of tokens to predict
    int32_t top_k;          // top k logits to sample from
    float top_p;            // nucleus sampling probability threshold
    float temp;             // temperature to adjust model's output distribution
    int32_t n_batch;        // number of predictions to generate in parallel
    float repeat_penalty;   // penalty factor for repeated tokens
    int32_t repeat_last_n;  // last n tokens to penalize
    float context_erase;    // percent of context to erase if we exceed the context window
};
#ifndef __cplusplus
typedef struct llmodel_prompt_context llmodel_prompt_context;
#endif

/**
 * Callback type for prompt processing.
 * @param token_id The token id of the prompt.
 * @return a bool indicating whether the model should keep processing.
 */
typedef bool (*llmodel_prompt_callback)(int32_t token_id);

/**
 * Callback type for prompt processing, version 2.
 * @param token The token of the prompt.
 * @param logit The logit of the prompt.
 * @param is_part_of_template True if part of prompt template prompt.
 * @return a bool indicating whether the model should keep processing.
 */
typedef bool (*llmodel_prompt_callback2)(const char *token, float logit, bool is_part_of_template);

/**
 * Callback type for response.
 * @param token_id The token id of the response.
 * @param response The response string. NOTE: a token_id of -1 indicates the string is an error string.
 * @return a bool indicating whether the model should keep generating.
 */
typedef bool (*llmodel_response_callback)(int32_t token_id, const char *response);

/**
 * Callback type for response, version 2.
 * @param token The token of the response.
 * @param token The logit of the response.
 * @return a bool indicating whether the model should keep generating.
 */
typedef bool (*llmodel_response_callback2)(const char *token, float logit);

/**
 * Callback type for recalculation of context.
 * @param whether the model is recalculating the context.
 * @return a bool indicating whether the model should keep generating.
 */
typedef bool (*llmodel_recalculate_callback)(bool is_recalculating);

/**
 * Callback type for recalculation of context, version 2.
 * @param whether the model is recalculating the context.
 * @return a bool indicating whether the model should keep generating.
 */
typedef llmodel_recalculate_callback llmodel_recalculate_callback2;

/**
 * Create a llmodel instance.
 * Recognises correct model type from file at model_path
 * @param model_path A string representing the path to the model file.
 * @return A pointer to the llmodel_model instance; NULL on error.
 */
DEPRECATED llmodel_model llmodel_model_create(const char *model_path);

/**
 * Create a llmodel instance.
 * Recognises correct model type from file at model_path
 * @param model_path A string representing the path to the model file; will only be used to detect model type.
 * @param build_variant A string representing the implementation to use (auto, default, avxonly, ...),
 * @param error A pointer to a llmodel_error; will only be set on error.
 * @return A pointer to the llmodel_model instance; NULL on error.
 */
llmodel_model llmodel_model_create2(const char *model_path, const char *build_variant, llmodel_error *error);

/**
 * Destroy a llmodel instance.
 * Recognises correct model type using type info
 * @param model a pointer to a llmodel_model instance.
 */
void llmodel_model_destroy(llmodel_model model);

/**
 * Load a model from a file.
 * @param model A pointer to the llmodel_model instance.
 * @param model_path A string representing the path to the model file.
 * @return true if the model was loaded successfully, false otherwise.
 */
bool llmodel_loadModel(llmodel_model model, const char *model_path);

/**
 * Check if a model is loaded.
 * @param model A pointer to the llmodel_model instance.
 * @return true if the model is loaded, false otherwise.
 */
bool llmodel_isModelLoaded(llmodel_model model);

/**
 * Get the size of the internal state of the model.
 * NOTE: This state data is specific to the type of model you have created.
 * @param model A pointer to the llmodel_model instance.
 * @return the size in bytes of the internal state of the model
 */
uint64_t llmodel_get_state_size(llmodel_model model);

/**
 * Saves the internal state of the model to the specified destination address.
 * NOTE: This state data is specific to the type of model you have created.
 * @param model A pointer to the llmodel_model instance.
 * @param dest A pointer to the destination.
 * @return the number of bytes copied
 */
uint64_t llmodel_save_state_data(llmodel_model model, uint8_t *dest);

/**
 * Restores the internal state of the model using data from the specified address.
 * NOTE: This state data is specific to the type of model you have created.
 * @param model A pointer to the llmodel_model instance.
 * @param src A pointer to the src.
 * @return the number of bytes read
 */
uint64_t llmodel_restore_state_data(llmodel_model model, const uint8_t *src);

/**
 * Generate a response using the model.
 * @param model A pointer to the llmodel_model instance.
 * @param prompt A string representing the input prompt.
 * @param prompt_callback A callback function for handling the processing of prompt.
 * @param response_callback A callback function for handling the generated response.
 * @param recalculate_callback A callback function for handling recalculation requests.
 * @param ctx A pointer to the llmodel_prompt_context structure.
 */
DEPRECATED void llmodel_prompt(llmodel_model model, const char *prompt,
                               llmodel_prompt_callback prompt_callback,
                               llmodel_response_callback response_callback,
                               llmodel_recalculate_callback recalculate_callback,
                               llmodel_prompt_context *ctx);

/**
 * Generate a response using the model, version 2.
 * @param model A pointer to the llmodel_model instance.
 * @param prompt A string representing the input prompt.
 * @param template_prefix A string representing the template prefix.
 * @param template_suffix A string representing the template suffix.
 * @param prompt_callback A callback function for handling the processing of prompt.
 * @param response_callback A callback function for handling the generated response.
 * @param recalculate_callback A callback function for handling recalculation requests.
 * @param ctx A pointer to the llmodel_prompt_context structure.
 * @param error A pointer to a llmodel_error; will only be set on error.
 * @return true on success, false on error
 */
bool llmodel_prompt2(llmodel_model model, const char *prompt,
                     const char *template_prefix, const char *template_suffix,
                     llmodel_prompt_callback2 prompt_callback,
                     llmodel_response_callback2 response_callback,
                     llmodel_recalculate_callback2 recalculate_callback,
                     llmodel_prompt_context *ctx, llmodel_error *error);

/**
 * Generate a response using the model.
 * @param model A pointer to the llmodel_model instance.
 * @param prompt A string representing the input prompt.
 * @param prompt_callback A callback function for handling the processing of prompt.
 * @param response_callback A callback function for handling the generated response.
 * @param recalculate_callback A callback function for handling recalculation requests.
 * @param ctx A pointer to the llmodel_prompt_context structure.
 */
void llmodel_prompt(llmodel_model model, const char *prompt,
                    llmodel_prompt_callback prompt_callback,
                    llmodel_response_callback response_callback,
                    llmodel_recalculate_callback recalculate_callback,
                    llmodel_prompt_context *ctx);

/**
 * Set the number of threads to be used by the model.
 * @param model A pointer to the llmodel_model instance.
 * @param n_threads The number of threads to be used.
 */
void llmodel_setThreadCount(llmodel_model model, int32_t n_threads);

/**
 * Get the number of threads currently being used by the model.
 * @param model A pointer to the llmodel_model instance.
 * @return The number of threads currently being used.
 */
int32_t llmodel_threadCount(llmodel_model model);

/**
 * Set llmodel implementation search path.
 * Default is "."
 * @param path The path to the llmodel implementation shared objects. This can be a single path or
 * a list of paths separated by ';' delimiter.
 */
void llmodel_set_implementation_search_path(const char *path);

/**
 * Get llmodel implementation search path.
 * @return The current search path; lifetime ends on next set llmodel_set_implementation_search_path() call.
 */
const char *llmodel_get_implementation_search_path();

#ifdef __cplusplus
}
#endif

#endif // LLMODEL_C_H
